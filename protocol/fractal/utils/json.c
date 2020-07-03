
/*
============================
Includes
============================
*/

#include "json.h"

#include <stdlib.h>
#include <string.h>

#include "logging.h"

/*
============================
Private Function Declarations
============================
*/

void skip_whitespace(char** str);
char next_char(char** str);
char next_alphanumeric_char(char** str);
char* read_escaped_string(char** str);

/*
============================
Public Functions
============================
*/

bool parse_json(char* str, json_t* json) {
    skip_whitespace(&str);
    if (*str != '{') {
        return false;
    }
    char c;

    kv_pair_t kv_pairs[100];
    int num_kv_pairs = 0;

    while ((c = next_alphanumeric_char(&str)) != '\0') {
        // LOG_INFO("CHAR: %c", c);

        // If we reached the end of the json
        if (c == '}') {
            break;
        }

        if (num_kv_pairs > 0) {
            if (c == ',') {
                c = next_alphanumeric_char(&str);
            } else {
                LOG_ERROR("JSON VALUE did not follow with ,! Had %c", *str);
                return false;
            }
        }

        kv_pair_t* kv = &kv_pairs[num_kv_pairs];
        num_kv_pairs++;

        if (*str != '\"') {
            LOG_ERROR("JSON KEY did not start with \"! Had %c", *str);
            return false;
        }

        kv->key = read_escaped_string(&str);
        // LOG_INFO("KEY BEGIN READ: %s", kv->key);

        if (next_alphanumeric_char(&str) != ':') {
            LOG_ERROR("JSON KEY did have a following :! Had %c", *str);
            return false;
        }

        if (next_alphanumeric_char(&str) != '\"') {
            if (memcmp(str, "true", 4) == 0) {
                str += 3;
                kv->type = JSON_BOOL;
                kv->bool_value = true;
            } else if (memcmp(str, "false", 5) == 0) {
                str += 4;
                kv->type = JSON_BOOL;
                kv->bool_value = false;
            } else if (memcmp(str, "null", 4) == 0) {
                str += 3;
                kv->type = JSON_NULL;
            } else if ('0' < *str && *str < '9') {
                kv->type = JSON_INT;
                kv->int_value = atoi(str);
                while ('0' <= *str && *str <= '9') {
                    str++;
                }
                str--;
            } else {
                kv = NULL;
                LOG_ERROR("JSON VALUE did not start with \"! Had %c", *str);
                return false;
            }
        } else {
            kv->type = JSON_STRING;
            kv->str_value = read_escaped_string(&str);
            // LOG_INFO("VALUE BEGIN READ: %s", kv->str_value);
        }

        /*
        if (kv == NULL) {
            LOG_INFO("VALUE ERROR");
            return false;
        } else {
            switch (kv->type) {
                case JSON_BOOL:
                    LOG_INFO("VALUE BEGIN: %s",
                                kv->bool_value ? "true" : "false");
                    break;
                case JSON_INT:
                    LOG_INFO("VALUE BEGIN: %d", kv->int_value);
                    break;
                case JSON_STRING:
                    LOG_INFO("VALUE BEGIN: \"%s\"", kv->str_value);
                    break;
            }
        }
        */
    }

    json->size = num_kv_pairs;
    json->pairs = malloc(sizeof(kv_pair_t) * num_kv_pairs);
    memcpy(json->pairs, kv_pairs, sizeof(kv_pair_t) * num_kv_pairs);

    /*
    for (int i = 0; i < json->size; i++) {
        kv_pair_t* kv = &json->pairs[i];
        LOG_INFO("KEY BEGIN: %s\n", kv->key);
        switch (kv->type) {
            case JSON_BOOL:
                LOG_INFO("VALUE BEGIN: %s", kv->bool_value ? "true" : "false");
                break;44
            case JSON_INT:
                LOG_INFO("VALUE BEGIN: %d", kv->int_value);
                break;
            case JSON_STRING:
                LOG_INFO("VALUE BEGIN: \"%s\"", kv->str_value);
                break;
        }
    }
    */

    return true;
}

kv_pair_t* get_kv(json_t* json, char* key) {
    for (int i = 0; i < json->size; i++) {
        if (strcmp(json->pairs[i].key, key) == 0) {
            return &json->pairs[i];
        }
    }
    return NULL;
}

void free_json(json_t json) {
    for (int i = 0; i < json.size; i++) {
        free(json.pairs[i].key);
        if (json.pairs[i].type == JSON_STRING) {
            free(json.pairs[i].str_value);
        }
    }
    free(json.pairs);
}

char* clone(char* str) {
    size_t len = strlen(str) + 1;
    char* ret_str = malloc(len);
    memcpy(ret_str, str, len);
    return ret_str;
}

/*
============================
Private Functions
============================
*/

void skip_whitespace(char** str) {
    while (**str == ' ' || **str == '\t' || **str == '\r' || **str == '\n') {
        (*str)++;
    }
}

char next_char(char** str) {
    (*str)++;
    return **str;
}

char next_alphanumeric_char(char** str) {
    next_char(str);
    skip_whitespace(str);
    return **str;
}

char* read_escaped_string(char** str) {
    char c;
    if (**str != '\"') {
        return NULL;
    }
    char* original_string = *str + 1;
    int str_len = 0;
    while ((c = next_char(str)) != '\"') {
        // LOG_INFO("CHAR: %c", c);
        str_len++;
    }
    char* return_str = malloc(str_len + 1);
    memcpy(return_str, original_string, str_len);
    return_str[str_len] = '\0';

    return return_str;
}
