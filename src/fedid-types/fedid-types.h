/*
 * Copyright (C) 2015-2021 IoT.bzh Company
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
#pragma once

#include <stdint.h>

struct json_object;

typedef struct
{
    unsigned refcount;
    int64_t stamp;
    const char *pseudo;
    const char *name;
    const char *email;
    const char *avatar;
    const char *company;
    const char **attrs;
} fedUserRawT;

typedef struct
{
    unsigned refcount;
    int64_t stamp;
    const char *idp;
    const char *fedkey;
    const char **attrs;
    const char *idpsid;
} fedSocialRawT;

fedUserRawT *fedUserCreate(const char *pseudo,
                           const char *email,
                           const char *name,
                           const char *avatar,
                           const char *company,
                           int64_t stamp);
fedUserRawT *fedUserAddRef(fedUserRawT *fedUser);
void fedUserUnRef(fedUserRawT *fedUser);
fedUserRawT *fedUserFromJSON(struct json_object *obj);
struct json_object *fedUserToJSON(const fedUserRawT *fedUser);

fedSocialRawT *fedSocialCreate(const char *idp,
                               const char *fedkey,
                               int64_t stamp);
fedSocialRawT *fedSocialAddRef(fedSocialRawT *fedSocial);
void fedSocialUnRef(fedSocialRawT *fedSocial);
fedSocialRawT *fedSocialFromJSON(struct json_object *obj);
struct json_object *fedSocialToJSON(const fedSocialRawT *fedSocial);

void fedIdpsFree(const char **fedIds);
