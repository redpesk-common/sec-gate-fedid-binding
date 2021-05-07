/*
 * Copyright (C) 2015-2021 IoT.bzh Company
 * Author "Fulup Ar Foll"
 *
 * $RP_BEGIN_LICENSE$
 * Commercial License Usage
 *  Licensees holding valid commercial IoT.bzh licenses may use this file in
 *  accordance with the commercial license agreement provided with the
 *  Software or, alternatively, in accordance with the terms contained in
 *  a written agreement between you and The IoT.bzh Company. For licensing terms
 *  and conditions see https://www.iot.bzh/terms-conditions. For further
 *  information use the contact form at https://www.iot.bzh/contact.
 *
 * GNU General Public License Usage
 *  Alternatively, this file may be used under the terms of the GNU General
 *  Public license version 3. This license is as published by the Free Software
 *  Foundation and appearing in the file LICENSE.GPLv3 included in the packaging
 *  of this file. Please review the following information to ensure the GNU
 *  General Public License requirements will be met
 *  https://www.gnu.org/licenses/gpl-3.0.html.
 * $RP_END_LICENSE$
*/
#define _GNU_SOURCE
#include "sqldb-glue.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// binding share a unique sqllite db with all clients
static sqlite3 *dbFd = NULL;
#define MAX_QUERIES 3

const char *sqlSchema =  // check with 'sqlite3 /xxx/fedid.db .tables'
    "CREATE TABLE fed_users"
    "('pseudo'  text NOT NULL"
    " ,'email'   text NOT NULL"
    " ,'name'  text"
    " ,'avatar'  text"
    " ,'company' text"
    " ,'tstamp' integer"
    " ,UNIQUE ('email')"
    " ,UNIQUE ('pseudo')"
    ");"
    "CREATE TABLE fed_keys"
    "('idp' text NOT NULL"
    " ,'fedkey' text NOT NULL"
    " ,'tstamp' integer"
    " ,'userid' INT UNSIGNED NOT NULL REFERENCES fed_users('rowid')"
    " ,UNIQUE ('idp', 'fedkey')"
    ");"
    "CREATE TABLE fed_label"
    "('label_uid' text NOT NULL"
    " ,'secret' text"
    " ,'epoc_create' integer"
    " ,'epoc_start'  integer"
    " ,'epoc_stop'   integer"
    " ,'count_max'   integer"
    " ,'count_used'  integer"
    " ,UNIQUE ('label_uid')"
    ");"
    "CREATE TABLE fed_user_label"
    " ('user'  INT UNSIGNED NOT NULL REFERENCES fed_users('id')"
    " ,'label' INT UNSIGNED NOT NULL REFERENCES fed_label('id')"
    " ,UNIQUE ('user', 'label')"
    ");";
     // end sqlSchema


void sqlFree (void *object) {
  sqlite3_free (object);
}

long sqlLastRow (void) {
   return sqlite3_last_insert_rowid (dbFd);
}

typedef struct {
   sqlite3_stmt *rqt;
   fedUserRawT user;
} sqlsqlCtxT;

static void sqlUserFreeCB (void *data) {
    sqlsqlCtxT *sqlCtx= (sqlsqlCtxT*)data;

    sqlite3_finalize (sqlCtx->rqt);
    free (sqlCtx);
}

int sqlUserAttrCheck (afb_req_t request, const char* attrLabel, const char *attrValue) {
    sqlite3_stmt *queryRqt=NULL;
    static char queryPattern[]=
        " select rowid from fed_users"
        " where %s='%s'"
        ";";
    int err, status;
    char *queryStr;
    int queryLen=asprintf (&queryStr, queryPattern, attrLabel, attrValue);
    if (queryLen<0) goto OnErrorExit;

    // compile request into byte code
    err= sqlite3_prepare_v3 (dbFd, queryStr, queryLen, 0, &queryRqt, NULL);
    if (err) goto OnErrorExit;

    // select should return one or no row
    switch (sqlite3_step(queryRqt)) {

        case SQLITE_DONE:
            status= FEDID_ATTR_FREE;
            break;

        case SQLITE_ROW:
            status= FEDID_ATTR_USED;
            break;

        default:
            goto OnErrorExit;
    }
    free (queryStr);
    return status;

OnErrorExit:
    AFB_REQ_ERROR (request, "[sql_error] %s (sqlUserAttrCheck)", sqlite3_errmsg(dbFd));
    sqlite3_finalize (queryRqt);
    return -1;
}

int sqlUserExist (afb_req_t request, const char* pseudo, const char* email) {
    sqlite3_stmt *queryRqt=NULL;
    static char queryPattern[]=
        " select rowid from fed_users"
        " where pseudo='%s' or email='%s'"
        ";";
    int err, status;
    char *queryStr;
    int queryLen=asprintf (&queryStr, queryPattern, pseudo, email);
    if (queryLen<0) goto OnErrorExit;

    // compile request into byte code
    err= sqlite3_prepare_v3 (dbFd, queryStr, queryLen, 0, &queryRqt, NULL);
    if (err) goto OnErrorExit;

    // select should return one or no row
    switch (sqlite3_step(queryRqt)) {

        case SQLITE_DONE:
            status= FEDID_ATTR_FREE;
            break;

        case SQLITE_ROW:
            status= FEDID_ATTR_USED;
            break;

        default:
            goto OnErrorExit;
    }
    free (queryStr);
    return status;

OnErrorExit:
    AFB_REQ_ERROR (request, "[sql_error] %s (sqlUserAttrCheck)", sqlite3_errmsg(dbFd));
    sqlite3_finalize (queryRqt);
    return -1;
}

int sqlRegisterFromSocial (afb_req_t request, const fedSocialRawT *fedSocial, fedUserRawT *fedUser) {
    sqlite3_stmt *queryRqt=NULL;

    static char queryPattern[]=
        " insert into fed_users(pseudo,email,name,avatar,company, tstamp)"
        " values('%s','%s','%s','%s','%s',%ld);"
        " insert into fed_keys (userid, idp, fedkey, tstamp)"
        " values(last_insert_rowid(),'%s','%s',%ld);"
        "";
    int err;
    unsigned long timeStamp= (unsigned long)time(NULL);
    char *queryStr;
    int queryLen=asprintf (&queryStr, queryPattern
        , fedUser->pseudo, fedUser->email, fedUser->name, fedUser->avatar, fedUser->company, timeStamp
        , fedSocial->idp, fedSocial->fedkey, timeStamp
    );
    if (queryLen<0) goto OnErrorExit;

    // Loop on every query commands
    for (const char *pzTail=queryStr; pzTail[0]; /* none */) {
        err= sqlite3_prepare_v3 (dbFd, pzTail, queryLen, 0, &queryRqt, &pzTail);
        if (err != SQLITE_OK) goto OnErrorExit;

        // handle space and comments
        if( !queryRqt ) continue;

        err= sqlite3_step(queryRqt);
        if (err != SQLITE_DONE) goto OnErrorExit;

        sqlite3_finalize (queryRqt);
    }
    free (queryStr);

    return 0;

OnErrorExit:
    AFB_REQ_ERROR (request, "[sql_error] %s (sqlRegisterFromSocial)", sqlite3_errmsg(dbFd));
    sqlite3_finalize (queryRqt);
    return -1;
}

int sqlFederateFromSocial (afb_req_t request, const fedSocialRawT *fedSocial, fedUserRawT *fedUser) {
    sqlite3_stmt *queryRqt=NULL;;

    static char queryPattern[]=
        " insert into fed_keys (userid, idp, fedkey, tstamp)"
        " values((select rowid from fed_users where email='%s' and pseudo='%s'),'%s','%s',%ld);"
        "";
    int err;
    unsigned long timeStamp= (unsigned long)time(NULL);
    char *queryStr;
    int queryLen=asprintf (&queryStr, queryPattern, fedUser->email, fedUser->pseudo, fedSocial->idp, fedSocial->fedkey, timeStamp
    );
    if (queryLen<0) goto OnErrorExit;

    // Loop on every query commands
    for (const char *pzTail=queryStr; pzTail[0]; /* none */) {
        err= sqlite3_prepare_v3 (dbFd, pzTail, queryLen, 0, &queryRqt, &pzTail);
        if (err != SQLITE_OK) goto OnErrorExit;

        // handle space and comments
        if( !queryRqt ) continue;

        err= sqlite3_step(queryRqt);
        if (err != SQLITE_DONE) goto OnErrorExit;

        sqlite3_finalize (queryRqt);
    }
    free (queryStr);

    return 0;

OnErrorExit:
    AFB_REQ_ERROR (request, "[sql_error] %s query=%s (sqlFederateFromSocial)", sqlite3_errmsg(dbFd), queryStr);
    sqlite3_finalize (queryRqt);
    return -1;
}

int sqlQueryFromSocial (afb_req_t request, const fedSocialRawT *fedSocial, afb_data_t reply[]) {
    sqlite3_stmt *queryRqt=NULL;
    static char queryPattern[]=
        " select usr.pseudo,usr.email,usr.name,usr.avatar,usr.company"
        " from fed_users usr, fed_keys keys"
        " where keys.userid=usr.rowid"
        " and keys.idp='%s' and keys.fedkey='%s'"
        ";";
    int err, count;
    char *queryStr;
    int queryLen=asprintf (&queryStr, queryPattern, fedSocial->idp, fedSocial->fedkey);
    if (queryLen<0) goto OnErrorExit;

    // compile request into byte code
    err= sqlite3_prepare_v3 (dbFd, queryStr, queryLen, 0, &queryRqt, NULL);
    if (err) goto OnErrorExit;

    // select should return one or no row
    switch (sqlite3_step(queryRqt)) {
        int colCount;

        case SQLITE_DONE:
        // user is unknown, registration requirer
        count= 0;
        break;

        case SQLITE_ROW:
        // user already exist retreive stored profil
        colCount=  sqlite3_column_count(queryRqt);
        if (colCount != 5) goto OnErrorExit;

        // allocate sqlUser handle and push it as a raw data
        sqlsqlCtxT *sqlCtx= calloc (1,sizeof(sqlsqlCtxT));
        sqlCtx->rqt= queryRqt;

        sqlCtx->user.pseudo= (char*)sqlite3_column_text(queryRqt,0);
        sqlCtx->user.email= (char*)sqlite3_column_text(queryRqt,1);
        sqlCtx->user.name= (char*)sqlite3_column_text(queryRqt,2);
        sqlCtx->user.avatar= (char*)sqlite3_column_text(queryRqt,3);
        sqlCtx->user.company= (char*)sqlite3_column_text(queryRqt,4);

        err= afb_create_data_raw(&reply[0], fedUserObjType, &sqlCtx->user, 0, sqlUserFreeCB, sqlCtx);
        if (err) goto OnErrorExit;

        count= 1; // on response
        break;

        default:
            goto OnErrorExit;
    }
    if (!count) {
        sqlite3_finalize (queryRqt);
        free (queryStr);
    }
    return count;

OnErrorExit:
    AFB_REQ_ERROR (request, "[sql_error] %s (sqlQueryFromSocial)", sqlite3_errmsg(dbFd));
    sqlite3_finalize (queryRqt);
    return -1;
}

// retreive IDP list from a feduser email or pseudo
int sqlUserLinkIdps (afb_req_t request, const char* pseudo, const char* email, afb_data_t reply[]) {
    sqlite3_stmt *queryRqt=NULL;
    static char queryPattern[]=
        " select keys.idp from fed_keys keys where keys.userid = "
        " (select rowid from fed_users usr where usr.pseudo='%s' or usr.email='%s')"
        ";";
    int err, status, count=0;
    char *queryStr;
    char **idps= malloc(FEDID_IDPS_MAX * sizeof(char*));

    int queryLen=asprintf (&queryStr, queryPattern, pseudo, email);
    if (queryLen<0) goto OnErrorExit;

    // compile request into byte code
    err= sqlite3_prepare_v3 (dbFd, queryStr, queryLen, 0, &queryRqt, NULL);
    if (err) goto OnErrorExit;

    // select should return one or no row
    for (status=sqlite3_step(queryRqt); status != SQLITE_DONE; status=sqlite3_step(queryRqt)) {

        if (status != SQLITE_ROW)   goto OnErrorExit;
        idps [count++]= strdup ((char*)sqlite3_column_text(queryRqt,0));
    }
    free (queryStr);
    sqlite3_finalize (queryRqt);

    // let build idplist data.
    if (count) {
        idps [count]=NULL;
        err= afb_create_data_raw(&reply[0], fedUserIdpsObjType, idps, 0, fedIdpsFreeCB, idps);
        if (err) goto OnErrorExit;
    }
    return count;

OnErrorExit:
    AFB_REQ_ERROR (request, "[sql_error] %s (sqlUserLinkIdps)", sqlite3_errmsg(dbFd));
    sqlite3_finalize (queryRqt);
    return -1;
}


int sqlCreate(const char *dbpath, char **errorMsg) {
  int rc;

  // db is already open
  if (dbFd) return 0;

  // check if DB exit;
  rc = sqlite3_open_v2(dbpath, &dbFd, SQLITE_OPEN_READONLY, NULL);

  // if db does not exist create it
  if (rc != SQLITE_OK) {
    rc = sqlite3_open_v2(dbpath, &dbFd, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK) {
      asprintf (errorMsg,"Fail to create SQLlite dbfile=%s", dbpath);
      goto OnExitError;
    }

    // populate db schema into newly create db
    // fprintf (stderr, "schema=%s\n",sqlSchema );
    rc = sqlite3_exec(dbFd, sqlSchema, NULL, 0, errorMsg);
    if (rc != SQLITE_OK) {
        remove (dbpath);
        goto OnExitError;
    }
  } else {
        // close readonly DB and reopen RW
        sqlite3_close_v2(dbFd);
        rc = sqlite3_open_v2(dbpath, &dbFd, SQLITE_OPEN_READWRITE, NULL);
        if (rc != SQLITE_OK) {
            asprintf (errorMsg,"Fail to create SQLlite dbfile=%s", dbpath);
            goto OnExitError;
        }
  }
  return 0;

OnExitError:
  dbFd = NULL;
  return -1;
}
