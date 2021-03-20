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

// binding share a unique sqllite db with all clients
static sqlite3 *dbFd = NULL;

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
    "('idp_uid' text NOT NULL"
    " ,'fed_key' text NOT NULL"
    " ,'tstamp' integer"
    " ,'userid' INT UNSIGNED NOT NULL REFERENCES fed_users('rowid')"
    " ,UNIQUE ('idp_uid', 'fed_key'), unique('userid')"
    ");"
    "CREATE TABLE fed_label"
    "('label_uid' text NOT NULL"
    " ,'secret' text"
    " ,'epoc_create' integer"
    " ,'epoc_start'  integer"
    " ,'epoc_stop'   integer"
    " ,'count_max'   integer"
    " ,'count_used'  integer"
    " ,'values'   blob"
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
} sqlRqtCtxT;

static void sqlUserFreeCB (void *data) {
    sqlRqtCtxT *rqtCtx= (sqlRqtCtxT*)data;

    sqlite3_finalize (rqtCtx->rqt);
    free (rqtCtx);
}

int sqlUserAttrCheck (afb_req_t request, const char* attrLabel, const char *attrValue) {
    sqlite3_stmt *queryRqt=NULL;
    static char queryPattern[]=
        " select rowid from fed_users"
        " where %s=%s"
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

int sqlRegisterFromSocial (afb_req_t request, const fedSocialRawT *fedSocial, fedUserRawT *fedUser) {
    sqlite3_stmt *queryRqt=NULL;
    int status;
    static char queryPattern[]=
        " insert into fed_keys (uip, fed_keys)"
        " values(%s,%s,%ld);"
        " insert into fed_users(id,pseudo,email,name,avatar,company,loa)"
        " values(last_insert_rowid(), %s,%s,%s,%s,%s,%d,%ld);"
        ";";
    int err;
    unsigned long timeStamp= (unsigned long)time(NULL);
    char *queryStr;
    int queryLen=asprintf (&queryStr, queryPattern
        , fedSocial->idp, fedSocial->fedkey, timeStamp
        , fedUser->pseudo, fedUser->email, fedUser->name, fedUser->avatar, fedUser->company, fedSocial->loa, timeStamp);
    if (queryLen<0) goto OnErrorExit;

    // compile request into byte code
    err= sqlite3_prepare_v3 (dbFd, queryStr, queryLen, 0, &queryRqt, NULL);
    if (err) goto OnErrorExit;

    // feduser
    if (SQLITE_DONE != sqlite3_step(queryRqt)) status= FEDID_USER_UNKNOWN;
    else status= FEDID_USER_EXIST;

    sqlite3_finalize (queryRqt);
    free (queryStr);
    return status;

OnErrorExit:
    AFB_REQ_ERROR (request, "[sql_error] %s (sqlRegisterFromSocial)", sqlite3_errmsg(dbFd));
    sqlite3_finalize (queryRqt);
    return -1;
}


int sqlQueryFromSocial (afb_req_t request, const fedSocialRawT *fedSocial, afb_data_t *response) {
    sqlite3_stmt *queryRqt=NULL;
    static char queryPattern[]=
        " select usr.pseudo,usr.email,usr.name,usr.avatar,usr.company"
        " from fed_users usr, fed_keys keys"
        " where keys.userid=usr.rowid"
        " and keys.idp_uid='%s' and keys.fed_key='%s'"
        ";";
    int err;
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
        *response=NULL;
        break;

        case SQLITE_ROW:
        // user already exist retreive stored profil
        colCount=  sqlite3_column_count(queryRqt);
        if (colCount != 6) goto OnErrorExit;

        // allocate sqlUser handle and push it as a raw data
        sqlRqtCtxT *rqtCtx= calloc (1,sizeof(sqlRqtCtxT));
        rqtCtx->rqt= queryRqt;

        rqtCtx->user.pseudo= (char*)sqlite3_column_text(queryRqt,0);
        rqtCtx->user.email= (char*)sqlite3_column_text(queryRqt,1);
        rqtCtx->user.name= (char*)sqlite3_column_text(queryRqt,2);
        rqtCtx->user.avatar= (char*)sqlite3_column_text(queryRqt,3);
        rqtCtx->user.company= (char*)sqlite3_column_text(queryRqt,4);

        err= afb_create_data_raw(response, fedUserObjType, &rqtCtx->user, 0, sqlUserFreeCB, rqtCtx);
        break;

        default:
            goto OnErrorExit;
    }
    sqlite3_finalize (queryRqt);
    free (queryStr);
    return 0;

OnErrorExit:
    AFB_REQ_ERROR (request, "[sql_error] %s (sqlQueryFromSocial)", sqlite3_errmsg(dbFd));
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
