/*
 * Copyright (C) 2015-2025 IoT.bzh Company
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

#include <stdio.h>
#include <string.h>

#include <json-c/json.h>
#include <rp-utils/rp-jsonc.h>

#define AFB_BINDING_VERSION 4
#include <afb/afb-binding.h>

#include "fedid-types/fedid-types-glue.h"
#include "fedid-types/fedid-types.h"
#include "sqldb-glue/sqldb-glue.h"

#if !defined(FEDID_SQLLITE_PATH)
#define FEDID_SQLLITE_PATH "/tmp/fedid.db"
#endif

static int reportSqlError(afb_req_t request, int rc, const char *info)
{
    if (rc == -1)
        return AFB_ERRNO_OUT_OF_MEMORY;
    AFB_REQ_ERROR(request, "[SQL:%s] %s", info ?: "?", sqlLastErrorMessage());
    return AFB_ERRNO_INTERNAL_ERROR;
}

static void fedPing(afb_req_t request, unsigned argc, afb_data_t const argv[])
{
    static int count = 0;
    int rc;
    char *response;
    afb_data_t reply;

    count++;
    AFB_REQ_DEBUG(request, "idp:fedPing count=%d", count);
    rc = asprintf(&response, "Pong=%d", count);
    if (rc >= 0)
        rc = afb_create_data_raw(&reply, AFB_PREDEFINED_TYPE_STRINGZ, response,
                                 (size_t)(rc + 1), free, response);
    if (rc < 0)
        afb_req_reply(request, AFB_ERRNO_OUT_OF_MEMORY, 0, NULL);
    else
        afb_req_reply(request, 0, 1, &reply);
}

/*
 * Implement verb "social-idps-check"
 *
 * Receive a JSON object with one or both of the fields: pseudo and email
 *
 * Look in the database for a user matching pseudo/email
 * and if found, returns the list of the IDP linked to that user.
 *
 * Return in status the count of listed IDP.
 * A negative status indicates an error.
 */
// Retrieve list of federated IDPs for a given pseudo/email
static void fedSocialIdps(afb_req_t request,
                          unsigned argc,
                          afb_data_t const argv[])
{
    afb_data_t data;
    const char *email, *pseudo;
    char **fedIdps;
    afb_data_t reply;
    int err, count, status = AFB_ERRNO_INVALID_REQUEST;
    json_object *obj;

    // check argument count
    if (argc != 1)
        goto end;

    // get object of request
    err = afb_req_param_convert(request, 0, AFB_PREDEFINED_TYPE_JSON_C, &data);
    if (err < 0)
        goto end;
    obj = afb_data_ro_pointer(data);

    // extract email and pseudo
    email = pseudo = NULL;
    err = rp_jsonc_unpack(obj, "{s?s s?s}", "email", &email, "pseudo", &pseudo);
    if (err < 0)
        goto end;

    // get the idps
    count = sqlUserLinkIdps(pseudo, email, &fedIdps);
    if (count < 0)
        status = reportSqlError(request, count, "sqlUserLinkIdps");
    else {
        err = afb_create_data_raw(&reply, fedUserIdpsObjType, fedIdps, 0,
                                  (void *)fedIdpsFree, fedIdps);
        if (err < 0)
            status = AFB_ERRNO_OUT_OF_MEMORY;
        else
            status = count;
    }

end:
    afb_req_reply(request, status, status >= 0, &reply);
}

// test if a user exists
static void fedUserExist(afb_req_t request,
                         unsigned argc,
                         afb_data_t const argv[])
{
    fedUserRawT *fedUser;
    afb_data_t data;
    int err, status = AFB_ERRNO_INVALID_REQUEST;

    // check argument count
    if (argc != 1)
        goto end;

    // get fedUser of request
    err = afb_req_param_convert(request, 0, fedUserObjType, &data);
    if (err < 0)
        goto end;
    fedUser = afb_data_ro_pointer(data);

    // perform the query
    err = sqlUserExist(fedUser->pseudo, fedUser->email);
    status = err >= 0 ? err : reportSqlError(request, err, "sqlUserExist");

end:
    afb_req_reply(request, status, 0, NULL);
}

/*
 * Implement verb "user-check"
 *
 * Receive a JSON object with 2 fields: label and value
 *
 * Checks in the table of users if an entry exists with the given
 * value.
 *
 * Return the status 1 if an entry is found or the status 0 otherwise.
 * A negative status indicates an error.
 */
static void fedUserAttr(afb_req_t request,
                        unsigned argc,
                        afb_data_t const argv[])
{
    afb_data_t data;
    const char *label, *value;
    json_object *obj;
    int err, status = AFB_ERRNO_INVALID_REQUEST;

    // check argument count
    if (argc != 1)
        goto end;

    // get object of request
    err = afb_req_param_convert(request, 0, AFB_PREDEFINED_TYPE_JSON_C, &data);
    if (err < 0)
        goto end;
    obj = afb_data_ro_pointer(data);

    // extract label and value
    err = rp_jsonc_unpack(obj, "{ss ss}", "label", &label, "value", &value);
    if (err < 0)
        goto end;

    // perform the query
    err = sqlUserAttrCheck(label, value);
    status = err >= 0 ? err : reportSqlError(request, err, "sqlUserAttrCheck");

end:
    afb_req_reply(request, status, 0, NULL);
}

// Create a new user profil and link it to its social id
static void fedUserRegister(afb_req_t request,
                            unsigned argc,
                            afb_data_t const argv[])
{
    afb_data_t data;
    fedUserRawT *fedUser;
    fedSocialRawT *fedSocial;
    int err, status = AFB_ERRNO_INVALID_REQUEST;

    // make sure we get right input parameters types
    if (argc != 2)
        goto end;

    // get fedUser
    err = afb_req_param_convert(request, 0, fedUserObjType, &data);
    if (err < 0)
        goto end;
    fedUser = afb_data_ro_pointer(data);

    // get fedSocial
    err = afb_req_param_convert(request, 1, fedSocialObjType, &data);
    if (err < 0)
        goto end;
    fedSocial = afb_data_ro_pointer(data);

    // register now
    err = sqlRegisterFromSocial(fedSocial, fedUser);
    status =
        err >= 0 ? err : reportSqlError(request, err, "sqlRegisterFromSocial");

end:
    afb_req_reply(request, status, 0, NULL);
}

/*
 * Link a social account with an existing federated user
 * Associates an existing user to a new social identity
 *
 * Takes 2 parameters:
 *  1. the user (at least the mail and pseudo)
 *  2. the sociality: idp and fedkey of the user for the idp
 *
 * Adds the sociality for the user.
 * Returns 0 on success or else a negative status
 */
static void fedUserFederate(afb_req_t request,
                            unsigned argc,
                            afb_data_t const argv[])
{
    afb_data_t data;
    fedUserRawT *fedUser;
    fedSocialRawT *fedSocial;
    int err, status = AFB_ERRNO_INVALID_REQUEST;

    // make sure we get right input parameters types
    if (argc != 2)
        goto end;

    // get fedUser
    err = afb_req_param_convert(request, 0, fedUserObjType, &data);
    if (err < 0)
        goto end;
    fedUser = afb_data_ro_pointer(data);

    // get fedSocial
    err = afb_req_param_convert(request, 1, fedSocialObjType, &data);
    if (err < 0)
        goto end;
    fedSocial = afb_data_ro_pointer(data);

    // register now
    err = sqlFederateFromSocial(fedSocial, fedUser);
    status =
        err >= 0 ? err : reportSqlError(request, err, "sqlFederateFromSocial");

end:
    afb_req_reply(request, status, 0, NULL);
}

/*
 * check if social id is already present within federation table
 *
 * Takes one paramater of type fedSocial or a JSON object like
 * { idp, fedkey }
 *
 * Query the database for a record holding a such item.
 * When a such record exists and a user matching that record exists,
 * returns the fields associated with the user as a fedUser type.
 *
 * The returned status can be:
 *  1 - the user is returned in associated data
 *  0 - no such user
 *  AFB_ERRNO_OUT_OF_MEMORY
 *  AFB_ERRNO_INTERNAL_ERROR
 */
static void fedSocialCheck(afb_req_t request,
                           unsigned argc,
                           afb_data_t const argv[])
{
    afb_data_t data, reply;
    fedSocialRawT *fedSocial;
    fedUserRawT *fedUser;
    int err, count = 0, status = AFB_ERRNO_INVALID_REQUEST;

    // make sure we get right input parameters types
    if (argc != 1)
        goto end;

    // get fedSocial
    err = afb_req_param_convert(request, 0, fedSocialObjType, &data);
    if (err < 0)
        goto end;
    fedSocial = afb_data_ro_pointer(data);

    // register now
    err = sqlQueryFromSocial(fedSocial, &fedUser);
    if (err < 0)
        status = reportSqlError(request, err, "sqlQueryFromSocial");
    else {
        status = 0;
        if (err > 0) {
            err = afb_create_data_raw(&reply, fedUserObjType, fedUser, 0,
                                      (void *)fedUserUnRef, fedUser);
            if (err < 0)
                status = AFB_ERRNO_OUT_OF_MEMORY;
            else
                count = 1;
        }
    }

end:
    afb_req_reply(request, status, count, &reply);
}

static int fedCtrl(afb_api_t api,
                   afb_ctlid_t ctlid,
                   afb_ctlarg_t ctlarg,
                   void *userdata)
{
    switch (ctlid) {
    case afb_ctlid_Pre_Init: {
        const char *dbpath = FEDID_SQLLITE_PATH;
        char *response;
        int err;
        json_object *config, *item;

        /* register types */
        err = fedUserObjTypesRegister();
        if (err) {
            AFB_ERROR("[register-type fail] Fail to register fed type");
            goto OnErrorExit;
        }

        /* get dbpath */
        config = ctlarg->pre_init.config;
        if (config != NULL &&
            json_object_object_get_ex(config, "dbpath", &item) &&
            json_object_is_type(item, json_type_string))
            dbpath = json_object_get_string(item);

        /* open db */
        err = sqlCreate(dbpath, &response);
        if (err) {
            AFB_ERROR(
                "[sql-backend fail] Fail to create/connect on sqlbackend");
            goto OnErrorExit;
        }

    } break;

    case afb_ctlid_Root_Entry:
    case afb_ctlid_Init:
    case afb_ctlid_Class_Ready:
    case afb_ctlid_Orphan_Event:
    case afb_ctlid_Exiting:
    default:
        break;
    }
    return 0;

OnErrorExit:
    return -1;
}

// clang-format off
static const afb_verb_t verbs[] = {
    {.verb = "ping",
     .callback = fedPing,
     .info = "ping pong check"},
    {.verb = "user-create",
     .callback = fedUserRegister,
     .info = "federate a new user from a new social ID"},
    {.verb = "user-federate",
     .callback = fedUserFederate,
     .info = "federate two social ID into one signed federated user"},
    {.verb = "user-check",
     .callback = fedUserAttr,
     .info = "check is a given attribute email,pseudo exist in DB"},
    {.verb = "user-exist",
     .callback = fedUserExist,
     .info = "check is any of pseudo/email is already federated"},
    {.verb = "social-check",
     .callback = fedSocialCheck,
     .info = "check if a social user is already registered"},
    {.verb = "social-idps",
     .callback = fedSocialIdps,
     .info = "retrieve existing federated IDPs for a given user"},
    {.verb = NULL}
};

const afb_binding_t afbBindingExport = {
    .api = "fedid",
    .info = "Federated Identity handling with an SQLlite backend",
    .verbs = verbs,
    .mainctl = fedCtrl,
    .provide_class = "identity",
    .noconcurrency = 1
};
// clang-format on
