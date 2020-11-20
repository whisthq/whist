#ifndef JSON_H
#define JSON_H
/**
 * Copyright Fractal Computers, Inc. 2020
 * @file json.h
 * @brief This file contains all the JSON helper functions.
============================
Usage
============================

You can use the functions here to handle JSONs in C. The function parse_json
will parse it into our custom JSON struct, and you can then use get_kv to
retrieve specific key and value pairs, the way you'd normally do in Python or
JavaScript.
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

typedef enum JsonType { JSON_BOOL, JSON_INT, JSON_STRING, JSON_NULL } JsonType;

typedef struct KVPair {
    JsonType type;
    char* key;
    union {
        char* str_value;
        int int_value;
        bool bool_value;
    };
} KVPair;

typedef struct Json {
    int size;
    KVPair* pairs;
} Json;

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
bool parse_json(char* str, Json* json);

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
KVPair* get_kv(Json* json, char* key);

/**
 * @brief                          Free the memory of a JSON stored in a JSON
 *                                 struct
 *
 * @param json                     The JSON struct to free
 */
void free_json(Json json);

/**
 * @brief                          Allocate new memory and clone a string
 *
 * @param str                      The string to clone
 *
 * @returns                        The cloned string
 */
char* clone(char* str);

#endif  // JSON_H
