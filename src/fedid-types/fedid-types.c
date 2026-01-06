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

#include "fedid-types.h"

#include <string.h>

#include <json-c/json.h>
#include <rp-utils/rp-jsonc.h>

/*******************************************************
 * fedUser
 *******************************************************/
fedUserRawT *fedUserAddRef(fedUserRawT *fedUser)
{
    __atomic_add_fetch(&fedUser->refcount, 1, __ATOMIC_RELAXED);
    return fedUser;
}

void fedUserUnRef(fedUserRawT *fedUser)
{
    if (fedUser != NULL &&
        !__atomic_sub_fetch(&fedUser->refcount, 1, __ATOMIC_RELAXED)) {
        free((void *)fedUser->pseudo);
        free((void *)fedUser->email);
        free((void *)fedUser->name);
        free((void *)fedUser->avatar);
        free((void *)fedUser->company);
        free(fedUser);
    }
}

fedUserRawT *fedUserCreate(const char *pseudo,
                           const char *email,
                           const char *name,
                           const char *avatar,
                           const char *company,
                           int64_t stamp)
{
    fedUserRawT *fedUser = calloc(1, sizeof *fedUser);
    if (fedUser != NULL) {
        fedUser->refcount = 1;
        fedUser->stamp = stamp;
        fedUser->pseudo = pseudo == NULL ? NULL : strdup(pseudo);
        fedUser->email = email == NULL ? NULL : strdup(email);
        fedUser->name = name == NULL ? NULL : strdup(name);
        fedUser->avatar = avatar == NULL ? NULL : strdup(avatar);
        fedUser->company = company == NULL ? NULL : strdup(company);
        if ((pseudo != NULL && fedUser->pseudo == NULL) ||
            (email != NULL && fedUser->email == NULL) ||
            (name != NULL && fedUser->name == NULL) ||
            (avatar != NULL && fedUser->avatar == NULL) ||
            (company != NULL && fedUser->company == NULL)) {
            fedUserUnRef(fedUser);
            fedUser = NULL;
        }
    }
    return fedUser;
}

fedUserRawT *fedUserFromJSON(struct json_object *obj)
{
    int rc;
    fedUserRawT fedUser;
    memset(&fedUser, 0, sizeof fedUser);
    rc = rp_jsonc_unpack(
        obj, "{s?s s?s s?s s?s s?s s?I}", "pseudo", &fedUser.pseudo, "email",
        &fedUser.email, "name", &fedUser.name, "avatar", &fedUser.avatar,
        "company", &fedUser.company, "stamp", &fedUser.stamp);
    return rc ? NULL
              : fedUserCreate(fedUser.pseudo, fedUser.email, fedUser.name,
                              fedUser.avatar, fedUser.company, fedUser.stamp);
}

struct json_object *fedUserToJSON(const fedUserRawT *fedUser)
{
    int rc;
    json_object *obj;
    rc = rp_jsonc_pack(&obj, "{ss* ss* ss* ss* ss* sI}", "pseudo",
                           fedUser->pseudo, "email", fedUser->email, "name",
                           fedUser->name, "avatar", fedUser->avatar, "company",
                           fedUser->company, "stamp", fedUser->stamp);
    return rc ? NULL : obj;
}

/*******************************************************
 * fedSocial
 *******************************************************/
fedSocialRawT *fedSocialAddRef(fedSocialRawT *fedSocial)
{
    __atomic_add_fetch(&fedSocial->refcount, 1, __ATOMIC_RELAXED);
    return fedSocial;
}

void fedSocialUnRef(fedSocialRawT *fedSocial)
{
    if (fedSocial != NULL &&
        !__atomic_sub_fetch(&fedSocial->refcount, 1, __ATOMIC_RELAXED)) {
        free((void *)fedSocial->idp);
        free((void *)fedSocial->fedkey);
        free(fedSocial);
    }
}

fedSocialRawT *fedSocialCreate(const char *idp,
                               const char *fedkey,
                               int64_t stamp)
{
    fedSocialRawT *fedSocial = calloc(1, sizeof *fedSocial);
    if (fedSocial != NULL) {
        fedSocial->refcount = 1;
        fedSocial->stamp = stamp;
        fedSocial->idp = idp == NULL ? NULL : strdup(idp);
        fedSocial->fedkey = fedkey == NULL ? NULL : strdup(fedkey);
        if ((idp != NULL && fedSocial->idp == NULL) ||
            (fedkey != NULL && fedSocial->fedkey == NULL)) {
            fedSocialUnRef(fedSocial);
            fedSocial = NULL;
        }
    }
    return fedSocial;
}

fedSocialRawT *fedSocialFromJSON(json_object *obj)
{
    const char *idp, *fedkey;
    int64_t stamp = 0;
    int rc;

    rc = rp_jsonc_unpack(obj, "{s?s s?s s?I}", "idp", &idp, "fedkey", &fedkey,
                         "stamp", &stamp);
    return rc ? NULL : fedSocialCreate(idp, fedkey, stamp);
}

struct json_object *fedSocialToJSON(const fedSocialRawT *fedSocial)
{
    int rc;
    json_object *obj;
    rc = rp_jsonc_pack(&obj, "{ss* ss* sI*}", "idp", fedSocial->idp, "fedkey",
                           fedSocial->fedkey, "stamp", fedSocial->stamp);
    return rc ? NULL : obj;
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
