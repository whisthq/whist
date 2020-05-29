#ifndef JSON_H
#define JSON_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file json.h
 * @brief This file contains all the JSON helper functions.
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

typedef enum json_type { JSON_BOOL, JSON_INT, JSON_STRING } json_type_t;

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
 * @brief                          Parse a string JSON into a JSON parsed struct

 * @param str                      JSON, in string, format to parse
 * @param json                     JSON struct to receive the parsed JSON
 *
 * @returns                        True if parsed successfully, else False
 */
bool parse_json(char* str, json_t* json);

/**
 * @brief                          Retrieve the value associated with a key in a
 *                                 JSON struct
 *
 * @param json                     The JSON struct to map key-value from
 * @param key                      The key to find the value of
 *
 * @returns                        A key-value pair struct of the found value
 *                                 for the provided key
 */
kv_pair_t* get_kv(json_t* json, char* key);

/**
 * @brief                          Free the memory of a JSON stored in a JSON
 *                                 struct
 *
 * @param json                     The JSON struct to free
 */
void free_json(json_t json);

/**
 * @brief                          Allocate new memory and clone a string
 *
 * @param str                      The string to clone
 *
 * @returns                        The cloned string
 */
char* clone(char* str);

#endif  // JSON_H
