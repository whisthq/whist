#ifndef JSON_H
#define JSON_H
/**
Copyright Fractal Computers, Inc. 2020
@file json.h
@brief JSON helper functions

============================
Usage
============================

*/


/*
============================
Includes
============================
*/

#include <stdbool.h>

#if defined(_WIN32)
#pragma warning(disable : 4201)
#endif

/*
============================
Custom Types
============================
*/


typedef enum json_type {
    JSON_BOOL,
    JSON_INT,
    JSON_STRING
} json_type_t;

typedef struct kv_pair {
    json_type_t type;
    char* key;
    union {
        char* str_value;
        int int_value;
        bool bool_value;
    };
} kv_pair_t;

typedef struct json {
    int size;
    kv_pair_t* pairs;
} json_t;

/*
============================
Public Functions
============================
*/

/**
 *
 * @param str
 * @param json
 * @return
 */
bool parse_json(char* str, json_t* json);

/**
 *
 * @param json
 * @param key
 * @return
 */
kv_pair_t* get_kv(json_t* json, char* key);

/**
 *
 * @param json
 */
void free_json(json_t json);
/**
 *
 * @param str
 * @return
 */
char* clone(char* str);

#endif
