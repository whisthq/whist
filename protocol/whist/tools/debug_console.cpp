/*
============================
Includes
============================
*/

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
using namespace std;

#include <assert.h>
extern "C" {
#include "debug_console.h"
#include <whist/network/udp.h>
#include <whist/utils/threads.h>
#include <whist/logging/logging.h>
#include "whist/utils/command_line.h"
#include "protocol_analyzer.h"
};

/*
============================
Defines
============================
*/

#ifdef _WIN32
#define strtok_r strtok_s
#endif

#define RED "\x1B[31m"
#define GRN "\x1B[32m"
#define YEL "\x1B[33m"
#define BLU "\x1B[34m"
#define MAG "\x1B[35m"
#define CYN "\x1B[36m"
#define WHT "\x1B[37m"
#define RESET "\x1B[0m"

static const int udp_packet_size_max = 65535;
static const int udp_packet_size_truncate = 65000;

/*
============================
Globals
============================
*/

// the UDP port debug console listens on
// if you use debug_console heavily, during developing you can set this to a positive
// value, then debug console will start by default
static int debug_console_listen_port = -1;
static SOCKET debug_console_listen_socket = INVALID_SOCKET;

static int atexit_handler_inserted = 0;
static int skip_last = 60;      // skip last few packets, since they might not have completely
                                // arrived.
static int num_records = 2000;  // num of records included in the report

static DebugConsoleOverrideValues g_override_values;

COMMAND_LINE_INT_OPTION(debug_console_listen_port, 0, "debug-console", 1024, 65535,
                        "Enable debug_console and related tools on the given port.")

/*
============================
Private Functions
============================
*/

static void init_overrided_values(void);
static int debug_console_thread(void *);
static int create_local_udp_listen_socket(SOCKET *sock, int port, int timeout_ms);

/*
============================
Public Function Implementations
============================
*/

DebugConsoleOverrideValues *get_debug_console_override_values() { return &g_override_values; }

int init_debug_console() {
    if (debug_console_listen_port == -1) return 0;
#ifdef USE_DEBUG_CONSOLE  // only enable debug console for debug build
    init_overrided_values();
    FATAL_ASSERT(create_local_udp_listen_socket(&debug_console_listen_socket,
                                                debug_console_listen_port, -1) == 0);
    whist_create_thread(debug_console_thread, "MultiThreadedDebugConsole", NULL);

    whist_analyzer_init();
#else
    // supress ci error
    UNUSED(init_overrided_values);
    UNUSED(create_local_udp_listen_socket);
    UNUSED(debug_console_thread);
#endif
    return 0;
}

int get_debug_console_listen_port() { return debug_console_listen_port; }

/*
============================
Private Function Implementations
============================
*/

static void init_overrided_values(void) {
    // if you need to force some value frequenly, you can set it here at compile time
    // g_override_values.no_minimize = 1;
    // g_overridee_values.verbose_log = 1;
}

static vector<string> string_to_vec(const char *s, const char *sp) {
    vector<string> res;
    string str = s;
    char *saveptr = (char *)str.c_str();
    char *p = strtok_r(saveptr, sp, &saveptr);
    while (p != NULL) {
        res.push_back(p);
        p = strtok_r(NULL, sp, &saveptr);
    }
    return res;
}

static string wrap_with_color(string str, string color) { return color + str + RESET; }

// this handler turns any exit() into abort() so that you can get the stack where exit() was called.
static void my_exit_handler(void) { abort(); }

// handle inserting of the atexit handler
static string handle_insert_atexit_handler() {
    if (atexit_handler_inserted) {
        return "already inserted before!";
    } else {
        atexit(my_exit_handler);
        atexit_handler_inserted = 1;
        return "handler inserted";
    }
}

// function to handle the set command
static string handle_set(vector<string> cmd) {
    FATAL_ASSERT(cmd[0] == "set");
    if (cmd.size() == 3) {
        int ok = 1;
        if (cmd[1] == "bitrate") {
            g_override_values.bitrate = stoi(cmd[2]);
        } else if (cmd[1] == "burst_bitrate") {
            g_override_values.burst_bitrate = stoi(cmd[2]);
        } else if (cmd[1] == "video_fec_ratio") {
            g_override_values.video_fec_ratio = stod(cmd[2]);
        } else if (cmd[1] == "audio_fec_ratio") {
            g_override_values.audio_fec_ratio = stod(cmd[2]);
        } else if (cmd[1] == "skip_last") {
            skip_last = stoi(cmd[2]);
        } else if (cmd[1] == "report_num") {
            num_records = stoi(cmd[2]);
        } else if (cmd[1] == "no_minimize") {
            g_override_values.no_minimize = stoi(cmd[2]);
        } else if (cmd[1] == "verbose_log") {
            g_override_values.verbose_log = stoi(cmd[2]);
        } else if (cmd[1] == "verbose_log_audio") {
            g_override_values.verbose_log_audio = stoi(cmd[2]);
        } else if (cmd[1] == "simulate_freeze") {
            g_override_values.simulate_freeze = stoi(cmd[2]);
        } else {
            ok = 0;
        }
        if (ok) return cmd[1] + " set as " + cmd[2];
    }
    return "";
}

// function to handle the report command
static string handle_report(vector<string> cmd) {
    int type = 0;
    bool more_format = 0;
    FATAL_ASSERT(cmd[0] == "report_video" || cmd[0] == "report_audio" ||
                 cmd[0] == "report_video_moreformat" || cmd[0] == "report_audio_moreformat");
    if (cmd[0] == "report_video") {
        type = 1;
        more_format = false;
    } else if (cmd[0] == "report_audio") {
        type = 0;
        more_format = false;
    } else if (cmd[0] == "report_video_moreformat") {
        type = 1;
        more_format = true;
    } else if (cmd[0] == "report_audio_moreformat") {
        type = 0;
        more_format = true;
    }
    string s = whist_analyzer_get_report(type, num_records, skip_last, more_format);
    if (cmd.size() > 1) {
        ofstream myfile;
        myfile.open(cmd[1].c_str());
        myfile << s;
        myfile.close();
        return "written to " + cmd[1];
    } else {
        return s;
    }
}

static string handle_info(vector<string> cmd) {
    FATAL_ASSERT(cmd[0] == "info");
    stringstream ss;
    ss << "bitrate=" << g_override_values.bitrate << endl;
    ss << "burst_bitrate=" << g_override_values.burst_bitrate << endl;
    ss << "audio_fec_ratio=" << g_override_values.audio_fec_ratio << endl;
    ss << "video_fec_ratio=" << g_override_values.video_fec_ratio << endl;
    ss << "no_minimize=" << g_override_values.no_minimize << endl;
    ss << "verbose_log=" << g_override_values.verbose_log << endl;
    ss << "verbose_log_audio=" << g_override_values.verbose_log_audio << endl;
    ss << "skip_last=" << skip_last << endl;
    ss << "report_num=" << num_records;
    return ss.str();
}

static int debug_console_thread(void *) {
    char buf[udp_packet_size_max];
    struct sockaddr_in addr;
    socklen_t slen = sizeof(addr);

    while (1) {
        // recv messages from clients
        int len = recvfrom(debug_console_listen_socket, buf, sizeof(buf) - 1, 0,
                           (struct sockaddr *)&addr, &slen);
        if (len < 0) {
            LOG_INFO("recv returrn %d errno %d", len, get_last_network_error());
            continue;
        }
        buf[len] = 0;

        // parse message into words, reply the parsed to client for confirm
        auto cmd = string_to_vec(buf, "\t\n\r ");
        string reply1 = "got message:";
        for (int i = 0; i < (int)cmd.size(); i++) {
            reply1 += "<";
            reply1 += cmd[i];
            reply1 += ">";
        }
        reply1 = wrap_with_color(reply1, GRN);
        reply1 += "\n";
        sendto(debug_console_listen_socket, reply1.c_str(), (int)reply1.size() + 1, 0,
               (struct sockaddr *)&addr, slen);

        if (cmd.size() == 0) continue;

        string reply2;
        if (cmd[0] == "set") {
            reply2 = handle_set(cmd);
        } else if (cmd[0] == "report_audio" || cmd[0] == "report_video" ||
                   cmd[0] == "report_audio_moreformat" || cmd[0] == "report_video_moreformat") {
            reply2 = handle_report(cmd);
        } else if (cmd[0] == "info") {
            reply2 = handle_info(cmd);
        } else if (cmd[0] == "insert_atexit_handler" || cmd[0] == "insert") {
            reply2 = handle_insert_atexit_handler();
        } else {
            reply2 = wrap_with_color("unrecongized command", RED);
        }

        // if the command handle return the an empty emssage warn with red "<null>"
        if (reply2 == "") reply2 = wrap_with_color("<null>", RED);

        if (reply2.size() > udp_packet_size_truncate)  // truncate message to near UDP max
        {
            reply2 = string(reply2.begin(), reply2.begin() + udp_packet_size_truncate);
        }
        reply2 = wrap_with_color(reply2, GRN);
        reply2 += "\n\n";
        sendto(debug_console_listen_socket, reply2.c_str(), (int)reply2.size() + 1, 0,
               (struct sockaddr *)&addr, slen);
        // int len3=sendto(listen_socket,  buf, len+1, 0, (struct sockaddr*)&addr, slen);
    }
    return 0;
}

static int create_local_udp_listen_socket(SOCKET *sock, int port, int timeout_ms) {
    LOG_INFO("Creating local listen UDP Socket");
    *sock = socketp_udp();
    if (*sock == INVALID_SOCKET) {
        LOG_ERROR("Failed to create UDP listen socket");
        return -1;
    }
    set_timeout(*sock, timeout_ms);

    // Bind the port to localhost
    struct sockaddr_in origin_addr;
    origin_addr.sin_family = AF_INET;
    origin_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    origin_addr.sin_port = htons((unsigned short)port);

    if (::bind(*sock, (struct sockaddr *)(&origin_addr), sizeof(origin_addr)) < 0) {
        LOG_ERROR("Failed to bind to port %d! errno=%d\n", port, get_last_network_error());
        closesocket(*sock);
        return -1;
    }

    return 0;
}
