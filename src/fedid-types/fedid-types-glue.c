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
 *  of this file. Please review the following information dest ensure the GNU
 *  General Public License requirements will be met
 *  https://www.gnu.org/licenses/gpl-3.0.html.
 * $RP_END_LICENSE$
 */
#define _GNU_SOURCE

#define AFB_BINDING_NO_ROOT
#define AFB_BINDING_VERSION 4

#include "fedid-types.h"
#include "fedid-types-glue.h"

#include <string.h>

#include <json-c/json.h>

#define FEDUSER_PROFIL_TYPENAME    "feduser-profil"
#define FEDSOCIAL_PROFIL_TYPENAME  "fedsocial-profil"
#define FEDUSER_IDPS_LIST_TYPENAME "feduser-idps"

afb_type_t fedUserObjType = NULL;
afb_type_t fedSocialObjType = NULL;
afb_type_t fedUserIdpsObjType = NULL;

/*******************************************************
 * fedUser
 *******************************************************/
static int userToJsonCB(void *ctx,
                          afb_data_t socialD,
                          afb_type_t jsonT,
                          afb_data_t *dest)
{
    const fedUserRawT *fedUser = afb_data_ro_pointer(socialD);
    json_object *socialJ = fedUserToJSON(fedUser);;
    if (socialJ != NULL) {
        if (0 == afb_create_data_raw(dest, AFB_PREDEFINED_TYPE_JSON_C, socialJ, 0,
                              (void *)json_object_put, socialJ))
           return 0;
        json_object_put(socialJ);
    }
    *dest = NULL;
    return -1;
}

static int userFromJsonCB(void *ctx,
                            afb_data_t jsonD,
                            afb_type_t socialT,
                            afb_data_t *dest)
{
    json_object *obj = (json_object *)afb_data_ro_pointer(jsonD);
    fedUserRawT *fedUser = fedUserFromJSON(obj);
    if (fedUser != NULL) {
        if (0 == afb_create_data_raw(dest, fedUserObjType, fedUser, 0,
                              (void *)fedUserUnRef, fedUser))
            return 0;
        fedUserUnRef(fedUser);
    }
    *dest = NULL;
    return -1;
}

/*******************************************************
 * fedSocial
 *******************************************************/
static int socialToJsonCB(void *ctx,
                          afb_data_t socialD,
                          afb_type_t jsonT,
                          afb_data_t *dest)
{
    const fedSocialRawT *fedSocial = afb_data_ro_pointer(socialD);
    json_object *socialJ = fedSocialToJSON(fedSocial);;
    if (socialJ != NULL) {
        if (0 == afb_create_data_raw(dest, AFB_PREDEFINED_TYPE_JSON_C, socialJ, 0,
                              (void *)json_object_put, socialJ))
           return 0;
        json_object_put(socialJ);
    }
    *dest = NULL;
    return -1;
}

static int socialFromJsonCB(void *ctx,
                            afb_data_t jsonD,
                            afb_type_t socialT,
                            afb_data_t *dest)
{
    json_object *obj = (json_object *)afb_data_ro_pointer(jsonD);
    fedSocialRawT *fedSocial = fedSocialFromJSON(obj);
    if (fedSocial != NULL) {
        if (0 == afb_create_data_raw(dest, fedSocialObjType, fedSocial, 0,
                              (void *)fedSocialUnRef, fedSocial))
            return 0;
        fedSocialUnRef(fedSocial);
    }
    *dest = NULL;
    return -1;
}

/*******************************************************
 * fedIdps
 *******************************************************/
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
    while (count) {
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
