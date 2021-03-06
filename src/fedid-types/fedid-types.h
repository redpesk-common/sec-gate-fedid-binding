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
#pragma once

#define FEDUSER_PROFIL_TYPE "feduser-profil"
#define FEDSOCIAL_PROFIL_TYPE "fedsocial-profil"
#define FEDUSER_IDPS_LIST "feduser-idps"
#define FEDID_IDPS_MAX 10

#define FEDID_TRAILLER (void*)-1

typedef enum {
    FEDID_ERROR= -100,
    FEDID_DONE=0,

    FEDID_USER_EXIST= 200,
    FEDID_USER_UNKNOWN= 404,
    FEDID_USER_REFUSED= 405,

    FEDID_ATTR_USED= 406,
    FEDID_ATTR_FREE= 407,

} fedidStatusE;

typedef struct {
    int slave;
    long stamp;
    const char *pseudo;
    const char *name;
    const char *email;
    const char *avatar;
    const char *company;
    const char **attrs;
} fedUserRawT;

typedef struct {
    int slave;
    long stamp;
    const char *idp;
    const char *fedkey;
    const char **attrs;
    const char *idpsid;
} fedSocialRawT;

int fedUserObjTypesRegister ();
extern afb_type_t fedUserObjType;
extern afb_type_t fedSocialObjType;
extern afb_type_t fedUserIdpsObjType;

void fedUserFreeCB (void *fedUserRawT);
void fedSocialFreeCB (void *fedSocialRawT);
void fedIdpsFreeCB (void *idpList);