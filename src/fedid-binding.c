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


#include "fedid-binding.h"
#include "sqldb-glue.h"

#include <stdio.h>
#include <string.h>
#include <json-c/json.h>

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

static void fedUserAttr(afb_req_x4_t request, unsigned nparams, afb_data_x4_t const params[]) {
    afb_data_t args[nparams];
    int err;


    // retreive feduser from API argv[0]
    // const char *values[nparams];
    // if (nparams != 2) goto OnErrorExit;
    // for (int idx=0; idx < 2; idx++) {
    //     err = afb_data_convert(params[idx], AFB_PREDEFINED_TYPE_STRINGZ, &args[idx]);
    //     values[idx]= afb_data_ro_pointer(args[idx]);
    //     if (err < 0) goto OnErrorExit;
    // }

    // Fulup TBD move from json to strings input

    err = afb_data_convert(params[0], AFB_PREDEFINED_TYPE_JSON_C, &args[0]);
    json_object *queryJ=  afb_data_ro_pointer(args[0]);
    if (!json_object_is_type(queryJ, json_type_array)  || json_object_array_length(queryJ) != 2) goto OnErrorExit;
    const char *values[2];
    values[0]= json_object_get_string (json_object_array_get_idx(queryJ,0));
    values[1]= json_object_get_string (json_object_array_get_idx(queryJ,1));

    err= sqlUserAttrCheck (request, values[0], values[1]);
    afb_req_reply(request, err, 0, NULL);

    return;

OnErrorExit:
    afb_req_reply(request, FEDID_ERROR, 0, NULL);  
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
    int err;
    afb_data_t argv[nparams];

    // make sure we get right input parameters types
    if (nparams != 2) goto OnErrorExit;
    err = afb_data_convert(params[0], fedUserObjType, &argv[0]);
    if (err < 0) goto OnErrorExit;
    err = afb_data_convert(params[1], fedSocialObjType, &argv[1]);
    if (err < 0) goto OnErrorExit;

    // extract raw object from params
    fedUserRawT *fedUser= afb_data_rw_pointer(argv[0]);
    fedSocialRawT *fedSocial= afb_data_ro_pointer(argv[1]);

    err= sqlRegisterFromSocial (request, fedSocial, fedUser);
    if (err < 0) goto OnErrorExit;
    afb_req_reply(request, err, 0, NULL);
    return;

OnErrorExit:
    afb_req_reply(request, FEDID_ERROR, 0, NULL);  
}

// check if social id is already present within federation table
static void fedSocialCheck(afb_req_t request, unsigned nparams, afb_data_t const params[]) {
    char *errorMsg;
    int err;
    afb_data_t reply;
    afb_data_t argv[nparams];

    if (nparams != 1) goto OnErrorExit;

    err = afb_data_convert(params[0], fedSocialObjType, &argv[0]);
    if (err < 0) {
      errorMsg= "[invalid-param] fail to retreive fedUser from argv[0]";
      afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, errorMsg, strlen(errorMsg) + 1, NULL, NULL);
      goto OnErrorExit;
    }

    // check if user exist
    const fedSocialRawT *fedSocial= afb_data_ro_pointer(argv[0]);
    err= sqlQueryFromSocial (request, fedSocial, &reply);
    if (err < 0) goto OnErrorExit;
    
    if (reply == NULL) afb_req_reply(request, FEDID_USER_UNKNOWN, 0, NULL);
    else  afb_req_reply(request, FEDID_USER_EXIST, 1, &reply);
    return;

OnErrorExit:
    afb_req_reply(request, FEDID_ERROR, 0, NULL);
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
      {.verb = "attr-check", .callback = fedUserAttr},
      {.verb = "social-check", .callback = fedSocialCheck},
      {.verb = "social-link", .callback = fedSocialLink},
      {.verb = NULL}};

  const struct afb_binding_v4 afbBindingV4 = {
      .api = "fedid",
      .specification = "Federated Identity handling with an SQLlite backend",
      .verbs = verbs,
      .mainctl = fedCtrl,
  };