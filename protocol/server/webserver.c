#include "../fractal/core/fractal.h"
#include "../fractal/utils/json.h"
#include "webserver.h"

static char* branch = NULL;
static bool is_autoupdate;
static bool already_obtained_vm_type = false;
static clock last_vm_info_check_time;
static bool is_using_stun;
static char* access_token = NULL;
bool is_trying_staging_protocol_info = false;

extern char identifier[FRACTAL_ENVIRONMENT_MAXLEN + 1];

void update_webserver_parameters() {
    if (already_obtained_vm_type && GetTimer(last_vm_info_check_time) < 30.0) {
        return;
    }

    bool will_try_staging = false;
    if (is_trying_staging_protocol_info) {
        will_try_staging = true;
    }
    is_trying_staging_protocol_info = false;

    if (!already_obtained_vm_type) {
        // Set Default Values
        is_autoupdate = true;
        is_using_stun = false;
    }

    char* resp_buf = NULL;
    size_t request_buf_maxlen = 256;
    size_t resp_buf_maxlen = 4800;

    LOG_INFO("GETTING JSON");

    char msg[request_buf_maxlen];
    sprintf(msg,
            "{\n"
            "    \"port\": %s\n"
            "}\n",
            identifier);

    if (!SendPostRequest(will_try_staging ? STAGING_HOST : PRODUCTION_HOST,
                         "/container/protocol_info", msg, NULL, &resp_buf, resp_buf_maxlen)) {
        already_obtained_vm_type = true;
        StartTimer(&last_vm_info_check_time);
        return;
    }

    if (!resp_buf) {
        already_obtained_vm_type = true;
        StartTimer(&last_vm_info_check_time);
        return;
    }

    LOG_INFO("Response body of length %d from POST request to webserver: %s", strlen(resp_buf),
             resp_buf);

    // Set Default Values
    is_autoupdate = true;
    is_using_stun = false;

    json_t json;
    if (!parse_json(resp_buf, &json)) {
        LOG_ERROR("Failed to parse JSON from /container/protocol_info");
        already_obtained_vm_type = true;
        StartTimer(&last_vm_info_check_time);
        return;
    }
    free(resp_buf);

    kv_pair_t* dev_value = get_kv(&json, "allow_autoupdate");
    kv_pair_t* branch_value = get_kv(&json, "branch");
    kv_pair_t* using_stun = get_kv(&json, "using_stun");
    kv_pair_t* access_token_value = get_kv(&json, "access_token");

    if (dev_value && branch_value) {
        if (dev_value->type != JSON_BOOL) {
            free_json(json);
            already_obtained_vm_type = true;
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

        if (access_token_value && access_token_value->type == JSON_STRING) {
            if (!access_token) {
                free(access_token);
            }
            access_token = clone(access_token_value->str_value);
        }
    } else {
        LOG_WARNING("COULD NOT GET JSON PARAMETERS FROM: %s", resp_buf);
    }

    free_json(json);
    if (is_autoupdate && !will_try_staging) {
        is_trying_staging_protocol_info = true;
        // This time trying the staging protocol info, if we haven't already
        update_webserver_parameters();
        return;
    } else {
        already_obtained_vm_type = true;
        StartTimer(&last_vm_info_check_time);
    }
}

char* get_branch() {
    if (!already_obtained_vm_type) {
        LOG_ERROR("Webserver parameters not updated!");
    }
    return branch;
}

bool get_using_stun() {
    if (!already_obtained_vm_type) {
        LOG_ERROR("Webserver parameters not updated!");
    }
    return is_using_stun;
}

bool allow_autoupdate() {
    if (!already_obtained_vm_type) {
        LOG_ERROR("Webserver parameters not updated!");
    }
    return is_autoupdate;
}

char* get_vm_password() { return "password1234567."; }

char* get_access_token() {
    if (!already_obtained_vm_type) {
        LOG_ERROR("Webserver parameters not updated!");
    }
    return access_token;
}
