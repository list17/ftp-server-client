//
// Created by list on 2019/10/16.
//

#ifndef FTP_SERVER_USER_H
#define FTP_SERVER_USER_H
typedef struct ftp_user{
    char *username;
    char *password;
    char *path;
}ftp_user;


typedef struct hash_tabel *users_t;


#endif //FTP_SERVER_USER_H
