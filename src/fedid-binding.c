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

static void fedIdPing(afb_req_t request, unsigned nparams, afb_data_t const params[]) {
  static int count = 0;
  char *response;
  afb_data_t reply;

  asprintf(&response, "Pong=%d", count++);
  AFB_REQ_NOTICE(request, "idp:fedIdPing count=%d", count);

  afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, response, strlen(response) + 1, free, NULL);
  afb_req_reply(request, 0, 1, &reply);

  return;
}

// Link in new social id with an existing profil id
static void fedIdLinkSocial(afb_req_t request, unsigned nparams, afb_data_t const params[]) {
  static int count = 0;
  char *response;
  afb_data_t reply;

  asprintf(&response, "Pong=%d", count++);
  AFB_REQ_NOTICE(request, "idp:fedIdPing count=%d", count);

  afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, response, strlen(response) + 1, free, NULL);
  afb_req_reply(request, 0, 1, &reply);

  return;
}

// Create a new user profil and link it to its social id
static void fedIdCreateProfil(afb_req_t request, unsigned nparams, afb_data_t const params[]) {
    char *errorMsg= "fail to create user profile (fedIdCreateProfil)";
    char query[256];
    int err;
    afb_data_t reply;
    afb_data_t args[nparams];
    unsigned long timeStamp= (unsigned long)time(NULL);

    // make sure we get right input parameters types
    if (nparams != 2) goto OnErrorExit;
    err = afb_data_convert(params[0], userObjType, &args[0]);
    if (err < 0) goto OnErrorExit;
    err = afb_data_convert(params[1], socialObjType, &args[1]);
    if (err < 0) goto OnErrorExit;

    // extract raw object from params
    const userObjRawT *userProfil= afb_data_ro_pointer(args[0]);
    const socialObjRawT *socialProfil= afb_data_ro_pointer(args[1]);

    snprintf(query, sizeof(query)
        , "INSERT INTO fed_profil(pseudo, email, avatar, company, loa, stamp) values(%s,%s,%s,%s,%ld,%ld);"
        , userProfil->pseudo, userProfil->email, userProfil->avatar, userProfil->company
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

static void fedidProfileFreeCB (void *data) {
    userObjRawT *userProfil= (userObjRawT*)data;

    if (userProfil->pseudo) free ((void*)userProfil->pseudo);
    if (userProfil->email) free ((void*)userProfil->email);
    if (userProfil->avatar) free ((void*)userProfil->avatar);
    if (userProfil->company) free ((void*)userProfil->company);
    free (userProfil);
}

// return an existing profil
static int fedIdProfilIdCB(void *ctx, int count, char **values, char **columns) {
  char *errorMsg= "[query-profil-fail] internal error in (fedIdSocialIdCB)";
  afb_req_t request = (afb_req_t)ctx;
  afb_data_t reply;
  int err;

  if (count == 0) afb_req_reply(request, 0, 0, NULL);
  else {
    userObjRawT *userProfil = calloc(1, sizeof(userObjRawT));

    for (int idx = 0; idx < count; idx++) {
        if (!strcasecmp(columns[idx], "id")) sscanf (values[idx], "%ld", &userProfil->id);
        else if (!strcasecmp(columns[idx], "pseudo")) userProfil->pseudo= strdup(values[idx]);
        else if (!strcasecmp(columns[idx], "email")) userProfil->email= strdup(values[idx]);
        else if (!strcasecmp(columns[idx], "avatar")) userProfil->avatar= strdup(values[idx]);
        else if (!strcasecmp(columns[idx], "company")) userProfil->company= strdup(values[idx]);
        else if (!strcasecmp(columns[idx], "loa")) sscanf (values[idx], "%ld", &userProfil->loa);
        else if (!strcasecmp(columns[idx], "stamp")) sscanf (values[idx], "%ld", &userProfil->stamp);
    }

    err= afb_create_data_raw(&reply, userObjType, userProfil, 0, fedidProfileFreeCB, userProfil);
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
static int fedIdSocialIdCB(void *ctx, int count, char **values, char **columns) {
  char *errorMsg= "[query-profil-fail] internal error in (fedIdSocialIdCB)";
  afb_req_t request = (afb_req_t)ctx;
  afb_data_t reply;
  int err;
  char query[256];

  // if profile does not exit respond -1 status with no params
  if (count != 1)
    afb_req_reply(request, 0, 0, NULL);
  else {
    err = snprintf(query, sizeof(query), "select 'user' from fed_profil where id=%s;", values[0]);
    if (err) {
      afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, sizeof(errorMsg), NULL, NULL);
      goto OnErrorExit;
    }

    err = sqlQuery(query, &errorMsg, fedIdProfilIdCB, (void*)request);
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
static void fedIdSocialId(afb_req_t request, unsigned nparams, afb_data_t const params[]) {
    int err;
    char *errorMsg;
    afb_data_t reply;
    afb_data_t args[nparams];
    char query[256];

    if (nparams != 2)
        goto OnErrorExit;

    for (int idx = 0; idx < nparams; idx++) {
        err = afb_data_convert(params[0], AFB_PREDEFINED_TYPE_STRINGZ, &args[0]);
        if (err < 0) goto OnErrorExit;
    }

    err = snprintf(
        query, sizeof(query),
        "select 'user' from fed_keys where idp_uid=%s and social_uid=%s;",
        (char *)afb_data_ro_pointer(args[0]),
        (char *)afb_data_ro_pointer(args[1])
    );
    if (err) goto OnErrorExit;

    err = sqlQuery(query, &errorMsg, fedIdSocialIdCB, (void*)request);
    if (err) goto OnErrorExit;

    return;

OnErrorExit:
    afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, strlen(errorMsg) + 1, sqlFree, errorMsg);
    afb_req_reply(request, -1, 1, &reply);
}

static int fedidCtrl(afb_api_t api, afb_ctlid_t ctlid, afb_ctlarg_t ctlarg, void *userdata) {
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

        err= userObjTypesRegister();
        if (err) {
            AFB_ERROR("[register-type fail] Fail to register fedid type");
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
      {.verb = "fedIdPing", .callback = fedIdPing},
      {.verb = "user-create", .callback = fedIdCreateProfil},
      {.verb = "social-check", .callback = fedIdSocialId},
      {.verb = "social-link", .callback = fedIdLinkSocial},
      {.verb = NULL}};

  const struct afb_binding_v4 afbBindingV4 = {
      .api = "fedid",
      .specification = "Federated Identity handling with an SQLlite backend",
      .verbs = verbs,
      .mainctl = fedidCtrl,
  };