/*
 * Copyright (C) 2015-2025 IoT.bzh Company
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
 *  of this file. Please review the following information dest ensure the GNU
 *  General Public License requirements will be met
 *  https://www.gnu.org/licenses/gpl-3.0.html.
 * $RP_END_LICENSE$
 */
#define _GNU_SOURCE

#define AFB_BINDING_NO_ROOT
#define AFB_BINDING_VERSION 4
#include <afb/afb-binding.h>

#include "fedid-types.h"

#include <json-c/json.h>
#include <rp-utils/rp-jsonc.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

afb_type_t fedUserObjType = NULL;
afb_type_t fedSocialObjType = NULL;
afb_type_t fedUserIdpsObjType = NULL;

/*******************************************************
* fedSocial
*******************************************************/
void fedSocialFree(fedSocialRawT *fedSocial)
{
    if (fedSocial != NULL) {
        if (!fedSocial->slave) {
            free((void *)fedSocial->idp);
            free((void *)fedSocial->fedkey);
        }
        free(fedSocial);
    }
}

static int socialToJsonCB(void *ctx,
                          afb_data_t socialD,
                          afb_type_t jsonT,
                          afb_data_t *dest)
{
    json_object *socialJ;
    const fedSocialRawT *fedSocial = afb_data_ro_pointer(socialD);

    // clang-format off
    int err = rp_jsonc_pack(&socialJ, "{ss ss si*}",
                            "idp", fedSocial->idp,
                            "fedkey", fedSocial->fedkey,
                            "stamp", fedSocial->stamp);
    // clang-format on
    if (err)
        goto OnErrorExit;

    err = afb_create_data_raw(dest, AFB_PREDEFINED_TYPE_JSON_C, socialJ, 0,
                              (void *)json_object_put, socialJ);
    if (err)
        goto OnErrorExit;
    return 0;

OnErrorExit:
    *dest = NULL;
    return -1;
}

static int socialFromJsonCB(void *ctx,
                            afb_data_t jsonD,
                            afb_type_t socialT,
                            afb_data_t *dest)
{
    int err;
    json_object *obj = (json_object *)afb_data_ro_pointer(jsonD);

    fedSocialRawT *fedSocial = calloc(1, sizeof(fedSocialRawT));
    if (fedSocial == NULL)
        goto OnErrorExit;

    fedSocial->slave = 1;
    // clang-format off
    err = rp_jsonc_unpack(obj, "{ss ss s?i}",
                          "idp", &fedSocial->idp,
                          "fedkey", &fedSocial->fedkey,
                          "stamp", &fedSocial->stamp);
    // clang-format on
    if (err) {
        fedSocialFree(fedSocial);
        goto OnErrorExit;
    }

    err = afb_create_data_raw(dest, fedSocialObjType, fedSocial, 0,
                              (void *)fedSocialFree, fedSocial);
    if (err)
        goto OnErrorExit;

    // link json object with raw dependency
    err = afb_data_dependency_add(*dest, jsonD);
    if (err) {
        afb_data_unref(*dest);
        goto OnErrorExit;
    }

    return 0;

OnErrorExit:
    *dest = NULL;
    return -1;
}

/*******************************************************
* fedUser
*******************************************************/
void fedUserFree(fedUserRawT *fedUser)
{
    if (fedUser != NULL) {
        if (!fedUser->slave) {
            free((void *)fedUser->pseudo);
            free((void *)fedUser->email);
            free((void *)fedUser->name);
            free((void *)fedUser->avatar);
            free((void *)fedUser->company);
        }
        free(fedUser);
    }
}

static int userToJsonCB(void *ctx,
                        afb_data_t userD,
                        afb_type_t jsonT,
                        afb_data_t *dest)
{
    json_object *userJ;
    const fedUserRawT *fedUser = afb_data_ro_pointer(userD);

    // clang-format off
    int err = rp_jsonc_pack(&userJ, "{ss ss* ss* ss* ss* si}",
                            "pseudo", fedUser->pseudo,
                            "email", fedUser->email,
                            "name", fedUser->name,
                            "avatar", fedUser->avatar,
                            "company", fedUser->company,
                            "stamp", fedUser->stamp);
    // clang-format on
    if (err)
        goto OnErrorExit;

    err = afb_create_data_raw(dest, AFB_PREDEFINED_TYPE_JSON_C, userJ, 0,
                              (void *)json_object_put, userJ);
    if (err)
        goto OnErrorExit;
    return 0;

OnErrorExit:
    *dest = NULL;
    return -1;
}

static int userFromJsonCB(void *ctx,
                          afb_data_t jsonD,
                          afb_type_t userT,
                          afb_data_t *dest)
{
    int err;
    json_object *obj = (json_object *)afb_data_ro_pointer(jsonD);

    fedUserRawT *fedUser = calloc(1, sizeof(fedUserRawT));
    if (fedUser == NULL)
        goto OnErrorExit;

    fedUser->slave = 1;
    // clang-format off
    err = rp_jsonc_unpack(obj, "{ss ss s?s s?s s?s s?i}",
                          "pseudo", &fedUser->pseudo,
                          "email", &fedUser->email,
                          "name", &fedUser->name,
                          "avatar", &fedUser->avatar,
                          "company", &fedUser->company,
                          "stamp", &fedUser->stamp);
    // clang-format on
    if (err) {
        fedUserFree(fedUser);
        goto OnErrorExit;
    }

    err = afb_create_data_raw(dest, fedUserObjType, fedUser, 0,
                              (void *)fedUserFree, fedUser);
    if (err)
        goto OnErrorExit;

    // link json object with raw dependency
    err = afb_data_dependency_add(*dest, jsonD);
    if (err) {
        afb_data_unref(*dest);
        goto OnErrorExit;
    }

    return 0;

OnErrorExit:
    *dest = NULL;
    return -1;
}

/*******************************************************
* fedIdps
*******************************************************/
void fedIdpsFree(const char **fedIdps)
{
    if (fedIdps != NULL) {
        const char **iter = fedIdps;
        while (*iter != NULL)
            free((void *)(*iter++));
        free(fedIdps);
    }
}

static int idpsToJsonCB(void *ctx,
                        afb_data_t ipdsD,
                        afb_type_t jsonT,
                        afb_data_t *dest)
{
        int err;
    json_object *obj, *str;
    const char **idps = afb_data_ro_pointer(ipdsD);

    // creates the array
    obj = json_object_new_array();
    if (obj == NULL)
        goto OnErrorExit;

    // fill the array
    while (*idps) {
            str = json_object_new_string(*idps++);
            if (str == NULL)
                goto OnErrorExitClean;
            json_object_array_add(obj, str);
    }

    // makes the data
    err = afb_create_data_raw(dest, AFB_PREDEFINED_TYPE_JSON_C, obj, 0,
                              (void *)json_object_put, obj);
    if (err)
        goto OnErrorExit;
    return 0;

OnErrorExitClean:
    json_object_put(obj);
OnErrorExit:
    *dest = NULL;
    return -1;
}

static int idpsFromJsonCB(void *ctx,
                          afb_data_t jsonD,
                          afb_type_t userT,
                          afb_data_t *dest)
{
    int err;
    size_t count;
    json_object *obj, *str;
    const char **idps = NULL;

    // get the JSON-C array
    obj = (json_object *)afb_data_ro_pointer(jsonD);
    if (!json_object_is_type(obj, json_type_array))
        goto OnErrorExit;

    // allocates the idps list
    count = json_object_array_length(obj);
    idps = calloc(count + 1, sizeof *idps);
    if (idps == NULL)
        goto OnErrorExit;

    // fill the idps list with copies
    while(count) {
        str = json_object_array_get_idx(obj, --count);
        if (!json_object_is_type(obj, json_type_string))
            goto OnErrorExit;
        idps[count] = strdup(json_object_get_string(str));
        if (idps[count] == NULL)
            goto OnErrorExit;
    }

    // creates the data
    err = afb_create_data_raw(dest, fedUserIdpsObjType, idps, 0,
                              (void *)fedIdpsFree, idps);
    if (err)
        goto OnErrorExit;

    return 0;

OnErrorExit:
    *dest = NULL;
    return -1;
}

/*******************************************************
* Registering
*******************************************************/
int fedUserObjTypesRegister()
{
    static int initialized = 0;
    int err;

    // type should be loaded only once per binder
    if (initialized)
        return 0;

    err = afb_type_register(&fedSocialObjType, FEDSOCIAL_PROFIL_TYPENAME, 0);
    if (err)
        goto OnErrorExit;
    afb_type_add_convert_to(fedSocialObjType, AFB_PREDEFINED_TYPE_JSON_C,
                            socialToJsonCB, NULL);
    afb_type_add_convert_from(fedSocialObjType, AFB_PREDEFINED_TYPE_JSON_C,
                              socialFromJsonCB, NULL);

    err = afb_type_register(&fedUserObjType, FEDUSER_PROFIL_TYPENAME, 0);
    if (err)
        goto OnErrorExit;
    afb_type_add_convert_to(fedUserObjType, AFB_PREDEFINED_TYPE_JSON_C,
                            userToJsonCB, NULL);
    afb_type_add_convert_from(fedUserObjType, AFB_PREDEFINED_TYPE_JSON_C,
                              userFromJsonCB, NULL);

    err = afb_type_register(&fedUserIdpsObjType, FEDUSER_IDPS_LIST_TYPENAME, 0);
    if (err)
        goto OnErrorExit;
    afb_type_add_convert_to(fedUserIdpsObjType, AFB_PREDEFINED_TYPE_JSON_C,
                            idpsToJsonCB, NULL);
    afb_type_add_convert_from(fedUserIdpsObjType, AFB_PREDEFINED_TYPE_JSON_C,
                              idpsFromJsonCB, NULL);


    initialized = 1;
    return 0;

OnErrorExit:
    return -1;
}
