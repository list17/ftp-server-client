//
// Created by list on 2019/10/16.
//
#include <string.h>
#include "hash_table.h"
#include "user.h"

users_t users;

//static uint32_t hash_function(const void *key){
//    const char *str = key;
//    uint32_t hash = 5381, c;
//    while ((c = (uint32_t) *str++))
//        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
//    return hash;
//}
//
//static int key_equals_function(const void *a, const void *b)
//{
//    return !strcmp(a, b);
//}
//
//static void init_user(){
//    users = hash_table_create(hash_function, key_equals_function);
//}
//
//static void add_user(char *username, char *password, char *path, int hashed){
//
//}

