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
 *  of this file. Please review the following information dest ensure the GNU
 *  General Public License requirements will be met
 *  https://www.gnu.org/licenses/gpl-3.0.html.
 * $RP_END_LICENSE$
 */
#define _GNU_SOURCE
#define AFB_BINDING_VERSION 4
#include <afb/afb-binding-x4.h>
#include <afb/afb-binding.h>

#include "fedid-types.h"

#include <json-c/json.h>
#include <wrap-json.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct {
   userObjRawT raw; 
   const json_object *json;
} userObjHandleT;

afb_type_t userObjType=NULL;

typedef struct {
   socialObjRawT raw; 
   const json_object *json;
} socialObjHandleT;

afb_type_t socialObjType=NULL;

static int socialToJsonCB (void *ctx,  afb_data_t socialD, afb_type_t jsonT, afb_data_t *dest) {
    assert(jsonT == AFB_PREDEFINED_TYPE_JSON_C);
    assert(afb_data_type (socialD) == socialObjType);
    json_object *socialJ;
    const socialObjRawT *social =  afb_data_ro_pointer(socialD);

    int err= wrap_json_pack (&socialJ, "{si si si ss ss}"
        ,"id", social->id
        ,"stamp", social->stamp
        ,"loa", social->loa
        ,"idp", social->idp
        ,"social", social->fedkey
    );
    if (err) goto OnErrorExit;

    err= afb_create_data_raw(dest, AFB_PREDEFINED_TYPE_JSON_C, socialJ, 0, (void*)json_object_put, socialJ);
    if (err) goto OnErrorExit;
    return 0;

OnErrorExit:
    return -1;    
}

static void socialFreeCB (void *data) {
    socialObjHandleT *social= (socialObjHandleT*)data;

    json_object_put((json_object*)social->json);
    free (data);
}

static int socialFromJsonCB (void *ctx,  afb_data_t jsonD, afb_type_t socialT, afb_data_t *dest) {
    int err;
    assert(socialObjType == socialT);
    assert(AFB_PREDEFINED_TYPE_JSON_C == afb_data_type (jsonD));

    // socialRaw depend on socialJson we have dest lock json object until raw object die.
    socialObjHandleT *social= calloc (1, sizeof(socialObjHandleT));
    social->json=afb_data_ro_pointer(jsonD);
    json_object_get ((json_object*)social->json);

    //socialObjRawT *social= calloc (1, sizeof(socialObjRawT));
    err= afb_create_data_raw (dest, socialT, &social->raw, sizeof(socialObjHandleT),socialFreeCB, social);
    if (err) goto OnErrorExit;

    err= wrap_json_unpack ((json_object*)afb_data_ro_pointer (jsonD), "{si si si ss ss}"
        ,"id", &social->raw.id
        ,"stamp", &social->raw.stamp
        ,"loa", &social->raw.loa
        ,"idp", &social->raw.idp
        ,"fedkey", &social->raw.fedkey
    );
    if (err) goto OnErrorExit;
    return 0;

OnErrorExit:
    return -1;    
}


static int userToJsonCB (void *ctx,  afb_data_t userD, afb_type_t jsonT, afb_data_t *dest) {
    assert(jsonT == AFB_PREDEFINED_TYPE_JSON_C);
    assert(afb_data_type (userD) == userObjType);
    json_object *userJ;
    const userObjRawT *user =  afb_data_ro_pointer(userD);

    int err= wrap_json_pack (&userJ, "{si si si ss* ss* ss*}"
        ,"id", user->id
        ,"loa", user->loa
        ,"stamp", user->stamp
        ,"email", user->email
        ,"avatar", user->avatar
        ,"company", user->company
    );
    if (err) goto OnErrorExit;

    err= afb_create_data_raw(dest, AFB_PREDEFINED_TYPE_JSON_C, userJ, 0, (void*)json_object_put, userJ);
    if (err) goto OnErrorExit;
    return 0;

OnErrorExit:
    return -1;    
}

static void userFreeCB (void *data) {
    userObjHandleT *user= (userObjHandleT*)data;

    json_object_put((json_object*)user->json);
    free (data);
}

static int userFromJsonCB (void *ctx,  afb_data_t jsonD, afb_type_t userT, afb_data_t *dest) {
    int err;
    assert(userObjType == userT);
    assert(AFB_PREDEFINED_TYPE_JSON_C == afb_data_type (jsonD));

    // userRaw depend on userJson we have dest lock json object until raw object die.
    userObjHandleT *user= calloc (1, sizeof(userObjHandleT));
    user->json=afb_data_ro_pointer(jsonD);
    json_object_get ((json_object*)user->json);

    //userObjRawT *user= calloc (1, sizeof(userObjRawT));
    err= afb_create_data_raw (dest, userT, &user->raw, sizeof(userObjHandleT),userFreeCB, user);
    if (err) goto OnErrorExit;

    err= wrap_json_unpack ((json_object*)afb_data_ro_pointer (jsonD), "{si si ss ss? ss? ss?}"
        ,"id", &user->raw.id
        ,"loa", &user->raw.loa
        ,"stamp", &user->raw.stamp
        ,"email", &user->raw.email
        ,"avatar", &user->raw.avatar
        ,"company", &user->raw.company
    );
    if (err) goto OnErrorExit;
    return 0;

OnErrorExit:
    return -1;    
}

int userObjTypesRegister () {
    int err;
    void *context=NULL;

    err= afb_type_register(&socialObjType, SOCIAL_PROFIL_TYPE, 0 /* not opac */);
    if (err) goto OnErrorExit;
    afb_type_add_convert_to (socialObjType, AFB_PREDEFINED_TYPE_JSON_C, socialToJsonCB, context);
    afb_type_add_convert_from (socialObjType, AFB_PREDEFINED_TYPE_JSON_C, socialFromJsonCB, context);

    err= afb_type_register(&userObjType, USER_PROFIL_TYPE, 0 /* not opac */);
    if (err) goto OnErrorExit;
    afb_type_add_convert_to (userObjType, AFB_PREDEFINED_TYPE_JSON_C, userToJsonCB, context);
    afb_type_add_convert_from (userObjType, AFB_PREDEFINED_TYPE_JSON_C, userFromJsonCB, context);

    return 0;

OnErrorExit:
    return 1;    
}