The database is made of 2 tables:

- fed\_users: table of users

- fed\_keys: table of idp and key for users

Users are referenced by them pseudo and/or email

Associated to any user, in the table fed\_keys, are
recorded different couples of idp and fedkey.

Operations are:

  check:     ATTR x VALUE -> BOOL
  exist:     PSEUDO? x EMAIL? -> BOOL
  register:  USER-DATA x IDP x FEDKEY -> NIL
  federate:  PSEUDO x EMAIL x IDP x FEDKEY -> NIL
  query:     IDP x FEDKEY -> USER-DATA
  list:      PSEUDO? x EMAIL? -> [IDP]

check attribute:

   NOT EMPTY(select rowid from fed_user where {attribute}='{value}')

user exists:

   NOT EMPTY(select rowid from fed_users where pseudo='{pseudo}' or email='{email}')

register from social:

   insert into fed_users(pseudo,email,name,avatar,company, tstamp)
            values('{pseudo}','{email}','{name}','{avatar}','{company}',{tstamp});
   insert into fed_keys (userid, idp, fedkey, tstamp)
            values(last_insert_rowid(),'{idp}','{fedkey}',{tstamp});

federate from social:

   insert into fed_keys (userid, idp, fedkey, tstamp)
            values((select rowid from fed_users where email='{email}' and "pseudo='{pseudo}'),
                    '{idp}','{fedkey}',{tstamp});

query from social:

   select usr.pseudo,usr.email,usr.name,usr.avatar,usr.company
        from fed_users usr, fed_keys keys
        where keys.userid=usr.rowid and keys.idp='{idp}' and keys.fedkey='{fedkey}'

get user link idps:

   select keys.idp from fed_keys keys
        where keys.userid = (select rowid from fed_users usr
                                where usr.pseudo='{pseudo}' or usr.email='{email}');


