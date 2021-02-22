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

// binding share a unique sqllite db with all clients
static sqlite3 *dbFd = NULL;

const char *sqlSchema =  // check with 'sqlite3 /xxx/fedid.db .tables'
    "CREATE TABLE fed_users"
    " ('id' INTEGER PRIMARY KEY AUTOINCREMENT"
    " ,'pseudo'  text NOT NULL"
    " ,'email'   text NOT NULL"
    " ,'name'  text"
    " ,'avatar'  text"
    " ,'company' text"
    " ,'loa' integer"
    " ,'stamp' integer"
    " ,'label_ids' blob"
    " ,UNIQUE ('email')"
    " ,UNIQUE ('pseudo')"
    ");"
    "CREATE TABLE fed_keys"
    " ('id' INTEGER PRIMARY KEY AUTOINCREMENT"
    " ,'idp_uid' text NOT NULL"
    " ,'fed_key' text NOT NULL"
    " ,'stamp' integer"
    " ,'user' INT UNSIGNED NOT NULL REFERENCES fed_users('id')"
    " ,UNIQUE ('idp_uid', 'fed_key'), unique('user')"
    ");"
    "CREATE TABLE fed_label"
    " ('id' INTEGER PRIMARY KEY AUTOINCREMENT"
    " ,'label_uid' text NOT NULL"
    " , 'loa' integer"
    " ,'epoc_create' integer"
    " ,'epoc_start'  integer"
    " ,'epoc_stop'   integer"
    " ,'count_max'   integer"
    " ,'count_used'  integer"
    " ,'values'   blob"
    " ,UNIQUE ('label_uid')"
    ");"; // end sqlSchema


void sqlFree (void *object) {
  sqlite3_free (object);
}

long sqlLastRow (void) {
   return sqlite3_last_insert_rowid (dbFd); 
}

int sqlQuery(const char *query, char **errorMsg, sqlQueryCbT callback, void *ctx) {
  int rc;
  rc = sqlite3_exec(dbFd, sqlSchema, callback, ctx, errorMsg);
  return rc;
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
  }
  return 0;

OnExitError:
  dbFd = NULL;
  return 1;
}
