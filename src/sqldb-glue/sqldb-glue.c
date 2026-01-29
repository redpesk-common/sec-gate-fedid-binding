/*
 * Copyright (C) 2015-2021 IoT.bzh Company
 * Author <dev-team@iot.bzh>
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

#include <assert.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if 0
#define PRINT(...) fprintf(stderr, "[FEDID-DB] " __VA_ARGS__)
#else
#define PRINT(...)
#endif

static const char *sqlSchema =  // check with 'sqlite3 /xxx/fedid.db .tables'
    "CREATE TABLE fed_users"
    "('pseudo'  text NOT NULL"
    ",'email'   text NOT NULL"
    ",'name'    text"
    ",'avatar'  text"
    ",'company' text"
    ",'tstamp'  integer"
    ",UNIQUE ('email')"
    ",UNIQUE ('pseudo')"
    ");"
    "CREATE TABLE fed_keys"
    "('idp'    text NOT NULL"
    ",'fedkey' text NOT NULL"
    ",'tstamp' integer"
    ",'userid' INT UNSIGNED NOT NULL REFERENCES fed_users('rowid')"
    ",UNIQUE ('idp', 'fedkey')"
    ");";
// end sqlSchema

// binding share a unique sqllite db with all clients
static sqlite3 *dbFd = NULL;
static char *lastError = NULL;

static void setLastError(const char *text)
{
    free(lastError);
    lastError = text == NULL ? NULL : strdup(text);
}

static void recordError()
{
    setLastError(sqlite3_errmsg(dbFd));
}

const char *sqlLastErrorMessage()
{
    return lastError != NULL ? lastError : "unknown error";
}

int sqlCreate(const char *dbpath, char **errorMsg)
{
    int rc;
    char *report;

    *errorMsg = NULL;

    // db is already open
    if (dbFd)
        return 0;

    // check if DB exit;
    rc = sqlite3_open_v2(dbpath, &dbFd, SQLITE_OPEN_READONLY, NULL);

    // if db does not exist create it
    if (rc != SQLITE_OK) {
        rc = sqlite3_open_v2(dbpath, &dbFd,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
        if (rc != SQLITE_OK) {
            asprintf(errorMsg, "Creation failed, %s", sqlite3_errstr(rc));
            goto OnExitError;
        }

        // populate db schema into newly create db
        PRINT("schema=%s\n",sqlSchema );
        report = NULL;
        rc = sqlite3_exec(dbFd, sqlSchema, NULL, 0, &report);
        if (rc != SQLITE_OK) {
            if (report != NULL) {
                *errorMsg = strdup(report);
                sqlite3_free(report);
            }
            sqlite3_close_v2(dbFd);
            remove(dbpath);
            goto OnExitError;
        }
    }
    else {
        // close readonly DB and reopen RW
        sqlite3_close_v2(dbFd);
        rc = sqlite3_open_v2(dbpath, &dbFd, SQLITE_OPEN_READWRITE, NULL);
        if (rc != SQLITE_OK) {
            asprintf(errorMsg, "Opening failed, ", sqlite3_errstr(rc));
            goto OnExitError;
        }
    }
    return 0;

OnExitError:
    dbFd = NULL;
    return -1;
}

int sqlUserAttrCheck(const char *attrLabel, const char *attrValue)
{
    static char queryPattern[] =
        "select rowid from fed_users"
        " where %s='%s'"
        ";";

    sqlite3_stmt *queryRqt;
    int err, status;
    char *queryStr;
    int queryLen = asprintf(&queryStr, queryPattern, attrLabel, attrValue ?: "");
    if (queryLen < 0)
        return -1;
    PRINT("[QUERY: %s]\n", queryStr);

    // compile request into byte code
    err = sqlite3_prepare_v3(dbFd, queryStr, queryLen, 0, &queryRqt, NULL);
    free(queryStr);
    if (err != SQLITE_OK) {
        recordError();
        return -2;
    }

    // select should return one or no row
    switch (sqlite3_step(queryRqt)) {
    case SQLITE_DONE:
        status = 0;
        break;

    case SQLITE_ROW:
        status = 1;
        break;

    default:
        recordError();
        status = -2;
    }
    sqlite3_finalize(queryRqt);
    return status;
}

int sqlUserExist(const char *pseudo, const char *email)
{
    int rc, result = 0;

    if (pseudo != NULL && *pseudo != '\0') {
        rc = sqlUserAttrCheck("pseudo", pseudo);
        if (rc > 0)
            result = 1;
    }
    if (result == 0 && email != NULL && *email != '\0') {
        rc = sqlUserAttrCheck("email", email);
        if (rc > 0)
            result = 1;
    }
    return result;
}

static int execCommands(char *commands)
{
    sqlite3_stmt *queryRqt;
    int err = SQLITE_OK;
    const char *pzTail = commands;
    while (*pzTail) {
        err = sqlite3_prepare_v3(dbFd, pzTail, -1, 0, &queryRqt, &pzTail);
        if (err != SQLITE_OK) {
            recordError();
            break;
        }
        if (queryRqt != NULL) {
            err = sqlite3_step(queryRqt);
            if (err != SQLITE_DONE) {
                recordError();
                sqlite3_finalize(queryRqt);
                break;
            }
            sqlite3_finalize(queryRqt);
            err = SQLITE_OK;
        }
    }
    free(commands);
    return err == SQLITE_OK ? 0 : -2;
}

int sqlRegisterFromSocial(const fedSocialRawT *fedSocial,
                          const fedUserRawT *fedUser)
{
    static char queryPattern[] =
        "insert into fed_users(pseudo,email,name,avatar,company, tstamp)"
        " values('%s','%s','%s','%s','%s',%ld)"
        ";"
        "insert into fed_keys (userid, idp, fedkey, tstamp)"
        " values(last_insert_rowid(),'%s','%s',%ld)"
        ";";

    int queryLen;
    unsigned long timeStamp = (unsigned long)time(NULL);
    const char *pseudo = fedUser->pseudo == NULL ? "" : fedUser->pseudo;
    const char *email = fedUser->email == NULL ? "" : fedUser->email;
    const char *name = fedUser->name == NULL ? "" : fedUser->name;
    const char *avatar = fedUser->avatar == NULL ? "" : fedUser->avatar;
    const char *company = fedUser->company == NULL ? "" : fedUser->company;
    char *queryStr;
    queryLen = asprintf(&queryStr, queryPattern, pseudo, email,
                        name, avatar, company, timeStamp,
                        fedSocial->idp, fedSocial->fedkey, timeStamp);
    if (queryLen < 0)
        return -1;
    PRINT("[QUERY: %s]\n", queryStr);

    return execCommands(queryStr);
}

int sqlFederateFromSocial(const fedSocialRawT *fedSocial,
                          const fedUserRawT *fedUser)
{
    static char queryPattern[] =
        "insert into fed_keys (userid, idp, fedkey, tstamp)"
        " values((select rowid from fed_users where email='%s'"
        " or pseudo='%s'),'%s','%s',%ld)"
        ";";

    unsigned long timeStamp = (unsigned long)time(NULL);
    const char *pseudo = fedUser->pseudo == NULL ? "NULL" : fedUser->pseudo;
    const char *email = fedUser->email == NULL ? "NULL" : fedUser->email;
    char *queryStr;
    int queryLen = asprintf(&queryStr, queryPattern, email, pseudo,
                            fedSocial->idp, fedSocial->fedkey, timeStamp);
    if (queryLen < 0)
        return -1;
    PRINT("[QUERY: %s]\n", queryStr);

    return execCommands(queryStr);
}

int sqlQueryFromSocial(const fedSocialRawT *fedSocial, fedUserRawT **fedUser)
{
    static char queryPattern[] =
        "select usr.pseudo,usr.email,usr.name,usr.avatar,usr.company,usr.tstamp"
        " from fed_users usr, fed_keys keys"
        " where keys.userid=usr.rowid"
        " and keys.idp='%s' and keys.fedkey='%s'"
        ";";

    sqlite3_stmt *queryRqt;
    int err, count, colCount;
    char *queryStr;
    fedUserRawT *user;
    int queryLen =
        asprintf(&queryStr, queryPattern, fedSocial->idp, fedSocial->fedkey);
    if (queryLen < 0)
        return -1;
    PRINT("[QUERY: %s]\n", queryStr);

    // compile request into byte code
    err = sqlite3_prepare_v3(dbFd, queryStr, queryLen, 0, &queryRqt, NULL);
    free(queryStr);
    if (err != SQLITE_OK) {
        recordError();
        return -2;
    }

    // select should return one or no row
    count = -2;
    switch (sqlite3_step(queryRqt)) {
    case SQLITE_DONE:
        // user is unknown, registration required
        count = 0;
        break;

    case SQLITE_ROW:
        // user already exist retrieve stored profil
        colCount = sqlite3_column_count(queryRqt);
        if (colCount != 6)
            break;

        // allocate user
        user = fedUserCreate((const char *)sqlite3_column_text(queryRqt, 0),
                             (const char *)sqlite3_column_text(queryRqt, 1),
                             (const char *)sqlite3_column_text(queryRqt, 2),
                             (const char *)sqlite3_column_text(queryRqt, 3),
                             (const char *)sqlite3_column_text(queryRqt, 4),
                             sqlite3_column_int64(queryRqt, 5));

        if (user == NULL)
            count = -1;
        else {
            count = 1;
            *fedUser = user;
        }
        break;

    default:
        recordError();
        break;
    }
    sqlite3_finalize(queryRqt);
    return count;
}

// retrieve IDP list from a feduser email or pseudo
int sqlUserLinkIdps(const char *pseudo, const char *email, char ***fedIdps)
{
    static char queryPattern[] =
        "select keys.idp from fed_keys keys where keys.userid ="
        " (select rowid from fed_users usr where usr.pseudo='%s'"
        " or usr.email='%s')"
        ";";

    sqlite3_stmt *queryRqt;
    int err, queryLen, status, count = 0;
    char *queryStr;
    char **idps;
    pseudo = pseudo == NULL ? "NULL" : pseudo;
    email = email == NULL ? "NULL" : email;
    queryLen = asprintf(&queryStr, queryPattern, pseudo, email);
    if (queryLen < 0)
        return -1;
    PRINT("[QUERY: %s]\n", queryStr);

    // compile request into byte code
    err = sqlite3_prepare_v3(dbFd, queryStr, queryLen, 0, &queryRqt, NULL);
    free(queryStr);
    if (err != SQLITE_OK) {
        recordError();
        return -2;
    }

    // first step, count the rows
    for (count = 0; sqlite3_step(queryRqt) == SQLITE_ROW; count++)
        ;

    // allocate and fill
    idps = calloc(1 + count, sizeof(char *));
    if (idps == NULL)
        count = -1;
    else if (sqlite3_reset(queryRqt) != SQLITE_OK) {
        recordError();
        count = -2;
    }
    else {
        for (count = 0; sqlite3_step(queryRqt) == SQLITE_ROW; count++) {
            idps[count] = strdup((char *)sqlite3_column_text(queryRqt, 0));
            if (idps[count] == NULL) {
                count = -1;
                break;
            }
        }
    }
    sqlite3_finalize(queryRqt);
    if (count < 0)
        free(idps);
    else
        *fedIdps = idps;
    return count;
}
