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
#define AFB_BINDING_VERSION 4
#include <afb/afb-binding.h>

#include "fedid-binding.h"
#include "sqldb-glue.h"

#include <stdio.h>
#include <string.h>

static void fedPing(afb_req_t request, unsigned nparams, afb_data_t const params[]) {
  static int count = 0;
  char *response;
  afb_data_t reply;

  asprintf(&response, "Pong=%d", count++);
  AFB_REQ_NOTICE(request, "idp:fedPing count=%d", count);

  afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, response, strlen(response) + 1, free, NULL);
  afb_req_reply(request, 0, 1, &reply);

  return;
}

// Link in new social id with an existing profil id
static void fedSocialLink(afb_req_t request, unsigned nparams, afb_data_t const params[]) {
  static int count = 0;
  char *response;
  afb_data_t reply;

  asprintf(&response, "Pong=%d", count++);
  AFB_REQ_NOTICE(request, "idp:fedPing count=%d", count);

  afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, response, strlen(response) + 1, free, NULL);
  afb_req_reply(request, 0, 1, &reply);

  return;
}

// Create a new user profil and link it to its social id
static void fedUserRegister(afb_req_t request, unsigned nparams, afb_data_t const params[]) {
    char *errorMsg= "fail to create user profile (fedUserRegister)";
    char query[256];
    int err;
    afb_data_t reply;
    afb_data_t argv[nparams];
    unsigned long timeStamp= (unsigned long)time(NULL);

    // make sure we get right input parameters types
    if (nparams != 2) goto OnErrorExit;
    err = afb_data_convert(params[0], fedUserObjType, &argv[0]);
    if (err < 0) goto OnErrorExit;
    err = afb_data_convert(params[1], fedSocialObjType, &argv[1]);
    if (err < 0) goto OnErrorExit;

    // extract raw object from params
    const fedUserRawT *fedUser= afb_data_ro_pointer(argv[0]);
    const fedSocialRawT *socialProfil= afb_data_ro_pointer(argv[1]);

    snprintf(query, sizeof(query)
        , "INSERT INTO fed_users(pseudo, email, name, avatar, company, loa, stamp) values(%s,%s,%s,%s,%s,%ld,%ld);"
        , fedUser->pseudo, fedUser->email, fedUser->name, fedUser->avatar, fedUser->company
        , socialProfil->loa, timeStamp
    );
    err = sqlQuery(query, &errorMsg, NULL, NULL);
    if (err) {
      afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, strlen(errorMsg) + 1, sqlFree, errorMsg);
      goto OnErrorExit;
    }
    long userid= sqlLastRow ();
    snprintf(query, sizeof(query)
        , "INSERT INTO fed_keys(idp_uid, fed_key, user, stamp) values(%s,%s,%ld,%ld);"
        , socialProfil->idp, socialProfil->fedkey, userid, timeStamp
    );
    err = sqlQuery(query, &errorMsg, NULL, NULL);
    if (err) {
      afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, strlen(errorMsg) + 1, sqlFree, errorMsg);
      goto OnErrorExit;
    }

    afb_req_reply(request, 0, 0, 0);
    return;

OnErrorExit:
    afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, strlen(errorMsg) + 1, free, NULL);
    afb_req_reply(request, -1, 1, &reply);  
}



// return an existing profil
static int fedUserIdCB(void *ctx, int count, char **values, char **columns) {
  char *errorMsg= "[query-profil-fail] internal error in (fedSocialCheckCB)";
  afb_req_t request = (afb_req_t)ctx;
  afb_data_t reply;
  int err;

  if (count == 0) afb_req_reply(request, 0, 0, NULL);
  else {
    fedUserRawT *fedUser = calloc(1, sizeof(fedUserRawT));

    for (int idx = 0; idx < count; idx++) {
        if (!strcasecmp(columns[idx], "id")) sscanf (values[idx], "%ld", &fedUser->id);
        else if (!strcasecmp(columns[idx], "pseudo")) fedUser->pseudo= strdup(values[idx]);
        else if (!strcasecmp(columns[idx], "email")) fedUser->email= strdup(values[idx]);
        else if (!strcasecmp(columns[idx], "name")) fedUser->name= strdup(values[idx]);
        else if (!strcasecmp(columns[idx], "avatar")) fedUser->avatar= strdup(values[idx]);
        else if (!strcasecmp(columns[idx], "company")) fedUser->company= strdup(values[idx]);
        else if (!strcasecmp(columns[idx], "loa")) sscanf (values[idx], "%ld", &fedUser->loa);
        else if (!strcasecmp(columns[idx], "stamp")) sscanf (values[idx], "%ld", &fedUser->stamp);
    }

    err= afb_create_data_raw(&reply, fedUserObjType, fedUser, 0, fedUserFreeCB, fedUser);
    if (err) goto OnErrorExit;
    afb_req_reply(request, 0, 1, &reply);
  }
  return 0;

OnErrorExit: 
    afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, sizeof(errorMsg), NULL, NULL);
    afb_req_reply(request, -1, 1, &reply);
    return -1;
}

// if a social id exist return it corresponding user profil
static int fedSocialCheckCB(void *ctx, int count, char **values, char **columns) {
  char *errorMsg= "[query-profil-fail] internal error in (fedSocialCheckCB)";
  afb_req_t request = (afb_req_t)ctx;
  afb_data_t reply;
  int err;
  char query[256];

  // if profile does not exit respond -1 status with no params
  if (count != 1)
    afb_req_reply(request, 0, 0, NULL);
  else {
    err = snprintf(query, sizeof(query), "select 'user' from fed_users where id=%s;", values[0]);
    if (err) {
      afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, sizeof(errorMsg), NULL, NULL);
      goto OnErrorExit;
    }

    err = sqlQuery(query, &errorMsg, fedUserIdCB, (void*)request);
    if (err) {
      afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, strlen(errorMsg) + 1, sqlFree, errorMsg);
      goto OnErrorExit;
    }
  }
  return 0;

OnErrorExit:
  afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, strlen(errorMsg) + 1, sqlFree, errorMsg);
  afb_req_reply(request, -1, 1, &reply);
  return 0;
}

// check if social id is already present within federation table
static void fedSocialCheck(afb_req_t request, unsigned nparams, afb_data_t const params[]) {
    char *errorMsg;
    int err;
    afb_data_t reply;
    afb_data_t argv[nparams];
    char query[256];

    if (nparams != 1) goto OnErrorExit;

    err = afb_data_convert(params[0], fedSocialObjType, &argv[0]);
    if (err < 0) {
      errorMsg= "[invalid-param] fail to retreive fedUser from argv[0]";
      afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, strlen(errorMsg) + 1, NULL, NULL);
      goto OnErrorExit;
    }
    const fedSocialRawT *fedSocial= afb_data_ro_pointer(argv[0]);

    err = snprintf(
        query, sizeof(query),
        "select 'user' from fed_keys where idp_uid=%s and social_uid=%s;", fedSocial->idp, fedSocial->fedkey
    );
    if (err) {
      afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, strlen(errorMsg) + 1, sqlFree, errorMsg);
      goto OnErrorExit;
    }

    err = sqlQuery(query, &errorMsg, fedSocialCheckCB, (void*)request);
    if (err) goto OnErrorExit;

    return;

OnErrorExit:
    afb_req_reply(request, -1, 1, &reply);
}

static int fedUniqueCB(void *ctx, int count, char **values, char **columns) {
  if (count >0 ) fprintf (stderr, "*** fedUniqueCB count=%d\n", count);
  return count;
}

// check if user email or pseudo is already in DB
static void fedUserUnique(afb_req_t request, unsigned nparams, afb_data_t const params[]) {
    char *errorMsg="[invalid-feduser] fail to retreive feduser from params (fedUserUnique)";
    int err, count;
    afb_data_t reply;
    afb_data_t argv[nparams];
    char query[256];

    if (nparams != 1) goto OnErrorExit;

    err = afb_data_convert(params[0], fedUserObjType, &argv[0]);
    if (err < 0) goto OnErrorExit;
    const fedUserRawT *fedUser= afb_data_ro_pointer(argv[0]);

    err = snprintf(
        query, sizeof(query),
        "select 'user' from fed_users where email=%s or pseudo=%s;", fedUser->email, fedUser->pseudo
    );
    if (err) goto OnErrorExit;

    count = sqlQuery(query, &errorMsg, fedUniqueCB, (void*)request);
    afb_req_reply(request, count, 0, NULL);

    return;

OnErrorExit:
    afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, strlen(errorMsg) + 1, NULL, NULL);
    afb_req_reply(request, -1, 1, &reply);
}

static int fedCtrl(afb_api_t api, afb_ctlid_t ctlid, afb_ctlarg_t ctlarg, void *userdata) {
    char *response;
    int err;

    switch (ctlid) {
    case afb_ctlid_Root_Entry:
        AFB_NOTICE("unexpected root entry");
        break;

    case afb_ctlid_Pre_Init:
        afb_api_provide_class(api, "identity");

        err = sqlCreate(FEDID_SQLLITE_PATH, &response);
        if (err) {
            AFB_ERROR("[sql-backend fail] Fail to create/connect on sqlbackend");
            goto OnErrorExit;
        }

        err= fedUserObjTypesRegister();
        if (err) {
            AFB_ERROR("[register-type fail] Fail to register fed type");
            goto OnErrorExit;
        }

        break;

    case afb_ctlid_Init:
        // afb_api_require_api(api, INIT_REQUIRE_API, 1);
        // afb_alias_api(afb_api_name(api), "fakename");
        break;

    case afb_ctlid_Class_Ready:
        AFB_NOTICE("hello binding has classes ready");
        break;

    case afb_ctlid_Orphan_Event:
        AFB_NOTICE("received orphan event %s", ctlarg->orphan_event.name);
        break;

    case afb_ctlid_Exiting:
        AFB_NOTICE("exiting code %d", ctlarg->exiting.code);
        break;

    default:
        break;
    }
    return 0;

OnErrorExit:
    return 1;
}

  const afb_verb_t verbs[] = {
      {.verb = "fedPing", .callback = fedPing},
      {.verb = "user-create", .callback = fedUserRegister},
      {.verb = "user-unique", .callback = fedUserUnique},
      {.verb = "social-check", .callback = fedSocialCheck},
      {.verb = "social-link", .callback = fedSocialLink},
      {.verb = NULL}};

  const struct afb_binding_v4 afbBindingV4 = {
      .api = "fed",
      .specification = "Federated Identity handling with an SQLlite backend",
      .verbs = verbs,
      .mainctl = fedCtrl,
  };