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

#define AFB_BINDING_NO_ROOT
#define AFB_BINDING_VERSION 4
#include <afb/afb-binding.h>

#include "fedid-types.h"

#include <json-c/json.h>
#include <wrap-json.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>

struct afb_type_x4 *fedUserObjType=NULL;
struct afb_type_x4 *fedSocialObjType=NULL;
struct afb_type_x4 *fedUserIdpsObjType=NULL;

void fedIdpsFreeCB (void *data) {
    char **idps= (char**)data;

    for (int idx=0; idps[idx]; idx++) {
        free (idps[idx]);
    }
    free (idps);
}

void fedUserFreeCB (void *data) {
    fedUserRawT *fedUser= (fedUserRawT*)data;
    if (!fedUser->slave) {
        if (fedUser->pseudo) free ((void*)fedUser->pseudo);
        if (fedUser->email) free ((void*)fedUser->email);
        if (fedUser->name) free ((void*)fedUser->name);
        if (fedUser->avatar) free ((void*)fedUser->avatar);
        if (fedUser->company) free ((void*)fedUser->company);
    }
    fedUser->slave=-1; // help debug
    free (fedUser);
}

void fedSocialFreeCB (void*ctx) {
	fedSocialRawT *fedSocial= (fedSocialRawT*)ctx;
    if (!fedSocial->slave) {
    	if (fedSocial->idp) free ((void*)  fedSocial->idp);
	    if (fedSocial->fedkey) free ((void*)  fedSocial->fedkey);
    }
    fedSocial->slave=-1; // help debug
	free (fedSocial);
}

static int socialToJsonCB (void *ctx,  afb_data_t socialD, afb_type_t jsonT, afb_data_t *dest) {
    json_object *socialJ;
    const fedSocialRawT *fedSocial =  afb_data_ro_pointer(socialD);

    int err= wrap_json_pack (&socialJ, "{ss ss si*}"
        ,"idp", fedSocial->idp
        ,"fedkey", fedSocial->fedkey
        ,"stamp", fedSocial->stamp
    );
    if (err) goto OnErrorExit;

    err= afb_create_data_raw(dest, AFB_PREDEFINED_TYPE_JSON_C, socialJ, 0, (void*)json_object_put, socialJ);
    if (err) goto OnErrorExit;
    return 0;

OnErrorExit:
    return -1;
}

static int socialFromJsonCB (void *ctx,  afb_data_t jsonD, afb_type_t socialT, afb_data_t *dest) {
    int err;

    // socialRaw depend on socialJson we have dest lock json object until raw object die.
    fedSocialRawT *fedSocial= calloc (1, sizeof(fedSocialRawT));

    err= wrap_json_unpack ((json_object*)afb_data_ro_pointer (jsonD), "{ss ss s?i}"
        ,"fedkey", &fedSocial->fedkey
        ,"idp", &fedSocial->idp
        ,"stamp", &fedSocial->stamp
    );
    if (err) goto OnErrorExit;

    err= afb_create_data_raw (dest, fedSocialObjType, &fedSocial, 0, fedSocialFreeCB, fedSocial);
    if (err) goto OnErrorExit;

    // link json object with raw dependency
    err= afb_data_dependency_add (*dest, jsonD);
    if (err) goto OnErrorExit;
    fedSocial->slave=1;

    return 0;

OnErrorExit:
    return -1;
}

static int userToJsonCB (void *ctx,  afb_data_t userD, afb_type_t jsonT, afb_data_t *dest) {
    json_object *userJ;
    const fedUserRawT *fedUser =  afb_data_ro_pointer(userD);

    int err= wrap_json_pack (&userJ, "{ss ss* ss* ss* ss* si}"
        ,"pseudo", fedUser->pseudo
        ,"email", fedUser->email
        ,"name", fedUser->name
        ,"avatar", fedUser->avatar
        ,"company", fedUser->company
        ,"stamp", fedUser->stamp
    );
    if (err) goto OnErrorExit;

    err= afb_create_data_raw(dest, AFB_PREDEFINED_TYPE_JSON_C, userJ, 0, (void*)json_object_put, userJ);
    if (err) goto OnErrorExit;
    return 0;

OnErrorExit:
    return -1;
}

static int userFromJsonCB (void *ctx,  afb_data_t jsonD, afb_type_t userT, afb_data_t *dest) {
    int err;

    // userRaw depend on userJson we have dest lock json object until raw object die.
    fedUserRawT *fedUser= calloc (1, sizeof(fedUserRawT));

    err= wrap_json_unpack ((json_object*)afb_data_ro_pointer (jsonD), "{ss ss s?s s?s s?s}"
        ,"pseudo", &fedUser->pseudo
        ,"email", &fedUser->email
        ,"name", &fedUser->name
        ,"avatar", &fedUser->avatar
        ,"company", &fedUser->company
    );
    if (err) goto OnErrorExit;

    err= afb_create_data_raw (dest, fedUserObjType, fedUser, 0, fedUserFreeCB, fedUser);
    if (err) goto OnErrorExit;

    // link json object with raw dependency
    err= afb_data_dependency_add (*dest, jsonD);
    fedUser->slave=1;
    if (err) goto OnErrorExit;

    return 0;

OnErrorExit:
    return -1;
}

int fedUserObjTypesRegister () {
    static int initialized=0;
    int err;
    void *context=NULL;

    // type should be loaded only once per binder
    if (initialized) return 0;

    err= afb_type_register(&fedSocialObjType, FEDSOCIAL_PROFIL_TYPE, 0 /* not opac */);
    if (err) goto OnErrorExit;
    afb_type_add_convert_to (fedSocialObjType, AFB_PREDEFINED_TYPE_JSON_C, socialToJsonCB, context);
    afb_type_add_convert_from (fedSocialObjType, AFB_PREDEFINED_TYPE_JSON_C, socialFromJsonCB, context);

    err= afb_type_register(&fedUserObjType, FEDUSER_PROFIL_TYPE, 0 /* not opac */);
    if (err) goto OnErrorExit;
    afb_type_add_convert_to (fedUserObjType, AFB_PREDEFINED_TYPE_JSON_C, userToJsonCB, context);
    afb_type_add_convert_from (fedUserObjType, AFB_PREDEFINED_TYPE_JSON_C, userFromJsonCB, context);

    err= afb_type_register(&fedUserIdpsObjType, FEDUSER_IDPS_LIST, 0 /* not opac */);
    if (err) goto OnErrorExit;

    initialized=1;
    return 0;

OnErrorExit:
    return 1;
}