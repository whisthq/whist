#include <fractal/core/fractal.h>
#include <fractal/utils/json.h>
#include "webserver.h"

static clock last_protocol_info_check_time;

extern char hex_aes_private_key[33];
extern char identifier[FRACTAL_IDENTIFIER_MAXLEN + 1];
extern char webserver_url[WEBSERVER_URL_MAXLEN + 1];

void update_webserver_parameters() {
    /*
        Update parameters received from the container/protocol_info
        endpoint
    */

    // Don't need to check more than once every 30 sec, unless no meaningful response
    if (get_timer(last_protocol_info_check_time) < 30.0) {
        return;
    }

    char* resp_buf = NULL;
    size_t resp_buf_maxlen = 4800;

    LOG_INFO("GETTING JSON from container/protocol_info");

    char* msg = (char*)safe_malloc(64 + strlen(identifier) + strlen(hex_aes_private_key));
    sprintf(msg,
            "{\n"
            "   \"identifier\" : %s,\n"
            "   \"private_key\" : \"%s\"\n"
            "}",
            identifier, hex_aes_private_key);

    if (!send_post_request(webserver_url, "/container/protocol_info", msg, &resp_buf,
                           resp_buf_maxlen)) {
        start_timer(&last_protocol_info_check_time);
        return;
    }

    free(msg);

    if (!resp_buf) {
        start_timer(&last_protocol_info_check_time);
        return;
    }

    // DO NOT LOG THIS IN PRODUCTION -- IT MAY CONTAIN SENSITIVE INFO
    // LIKE PRIVATE KEYS
    /* LOG_INFO("Response body of length %d from POST request to webserver: %s", strlen(resp_buf),
     */
    /* resp_buf); */

    Json json;
    if (!parse_json(resp_buf, &json)) {
        LOG_ERROR("Failed to parse JSON from /container/protocol_info");
        start_timer(&last_protocol_info_check_time);
        return;
    }
    free(resp_buf);

    free_json(json);

    start_timer(&last_protocol_info_check_time);
}

bool get_using_stun() { return false; }
