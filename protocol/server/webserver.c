#include "../fractal/core/fractal.h"
#include "../fractal/utils/json.h"
#include "webserver.h"

static char* branch = NULL;
static bool is_autoupdate = true;
static clock last_vm_info_check_time;
static bool is_using_stun = false;
static char* container_id = NULL;
static char* user_id = NULL;

extern char hex_aes_private_key[33];
extern char identifier[FRACTAL_ENVIRONMENT_MAXLEN + 1];
extern char webserver_url[MAX_WEBSERVER_URL_LEN + 1];

void update_webserver_parameters(bool) {
    /*
        Update parameters received from the container/protocol_info
        endpoint
    */

    // Don't need to check more than once every 30 sec, unless no meaningful response
    if (container_id && GetTimer(last_vm_info_check_time) < 30.0) {
        return;
    }

    // Set Default Values
    is_autoupdate = true;
    is_using_stun = false;

    char* resp_buf = NULL;
    size_t resp_buf_maxlen = 4800;

    LOG_INFO("GETTING JSON");

    char* msg = (char*)malloc(64 + strlen(identifier) + strlen(hex_aes_private_key));
    sprintf(msg,
            "{\n"
            "   \"identifier\" : %s,\n"
            "   \"private_key\" : \"%s\"\n"
            "}",
            identifier, hex_aes_private_key);

    if (!SendPostRequest(webserver_url, "/container/protocol_info", msg, &resp_buf,
                         resp_buf_maxlen)) {
        StartTimer(&last_vm_info_check_time);
        return;
    }

    free(msg);

    if (!resp_buf) {
        StartTimer(&last_vm_info_check_time);
        return;
    }

    // DO NOT LOG THIS IN PRODUCTION -- IT MAY CONTAIN SENSITIVE INFO
    // LIKE PRIVATE KEYS
    /* LOG_INFO("Response body of length %d from POST request to webserver: %s", strlen(resp_buf),
     */
    /* resp_buf); */

    json_t json;
    if (!parse_json(resp_buf, &json)) {
        LOG_ERROR("Failed to parse JSON from /container/protocol_info");
        StartTimer(&last_vm_info_check_time);
        return;
    }
    free(resp_buf);

    kv_pair_t* dev_value = get_kv(&json, "allow_autoupdate");
    kv_pair_t* branch_value = get_kv(&json, "branch");
    kv_pair_t* using_stun = get_kv(&json, "using_stun");
    kv_pair_t* container_id_value = get_kv(&json, "container_id");
    kv_pair_t* user_id_value = get_kv(&json, "user_id");

    if (dev_value && branch_value) {
        if (dev_value->type != JSON_BOOL) {
            free_json(json);
            StartTimer(&last_vm_info_check_time);
            return;
        }

        is_autoupdate = dev_value->bool_value;

        if (branch) {
            free(branch);
        }
        if (branch_value->type == JSON_STRING) {
            branch = clone(branch_value->str_value);
        } else if (branch_value->type == JSON_NULL) {
            branch = clone("[NULL]");
        } else {
            branch = clone("");
        }

        LOG_INFO("Is Dev? %s", dev_value->bool_value ? "true" : "false");
        LOG_INFO("Branch: %s", branch);

        if (using_stun && using_stun->type == JSON_BOOL) {
            LOG_INFO("Using Stun: %s", using_stun->bool_value ? "Yes" : "No");
            is_using_stun = using_stun->bool_value;
        }

        if (container_id_value && container_id_value->type == JSON_STRING) {
            if (container_id) {
                free(container_id);
            }
            container_id = clone(container_id_value->str_value);
        }

        if (user_id_value && user_id_value->type == JSON_STRING) {
            if (user_id) {
                free(user_id);
            }
            user_id = clone(user_id_value->str_value);
        }

    } else {
        LOG_WARNING("COULD NOT GET JSON PARAMETERS FROM: %s", resp_buf);
    }

    free_json(json);

    StartTimer(&last_vm_info_check_time);
}

char* get_branch() {
    return branch;
}

bool get_using_stun() {
    return is_using_stun;
}

bool allow_autoupdate() {
    return is_autoupdate;
}

char* get_vm_password() { return "password1234567."; }

char* get_container_id() {
    return container_id;
}

char* get_user_id() {
    return user_id;
}
