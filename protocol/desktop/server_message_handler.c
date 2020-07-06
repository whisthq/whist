/**
 * Copyright Fractal Computers, Inc. 2020
 * @file server_message_handler.c
 * @brief This file contains all the code for client-side processing of messages
 *        received from the server
============================
Usage
============================

handleServerMessage() must be called on any received message from the server.
Any action trigged a server message must be initiated in network.c.
*/

#include "server_message_handler.h"

#include <stddef.h>

#include "../fractal/core/fractal.h"
#include "../fractal/utils/clock.h"
#include "../fractal/utils/logging.h"
#include "desktop_utils.h"
#include "server_message_handler.h"

#include <stddef.h>

extern char filename[300];
extern char username[50];
extern bool exiting;
extern int audio_frequency;
extern volatile bool received_server_init_message;
extern volatile bool is_timing_latency;
extern volatile clock latency_timer;
extern volatile int ping_id;
extern volatile int ping_failures;
extern volatile int try_amount;
extern int client_id;

static int handleInitMessage(FractalServerMessage *fmsg, size_t fmsg_size);
static int handlePongMessage(FractalServerMessage *fmsg, size_t fmsg_size);
static int handleQuitMessage(FractalServerMessage *fmsg, size_t fmsg_size);
static int handleAudioFrequencyMessage(FractalServerMessage *fmsg,
                                       size_t fmsg_size);
static int handleClipboardMessage(FractalServerMessage *fmsg, size_t fmsg_size);

int handleServerMessage(FractalServerMessage *fmsg, size_t fmsg_size) {
    switch (fmsg->type) {
        case MESSAGE_INIT:
            return handleInitMessage(fmsg, fmsg_size);
        case MESSAGE_PONG:
            return handlePongMessage(fmsg, fmsg_size);
        case SMESSAGE_QUIT:
            return handleQuitMessage(fmsg, fmsg_size);
        case MESSAGE_AUDIO_FREQUENCY:
            return handleAudioFrequencyMessage(fmsg, fmsg_size);
        case SMESSAGE_CLIPBOARD:
            return handleClipboardMessage(fmsg, fmsg_size);
        default:
            LOG_WARNING("Unknown FractalServerMessage Received");
            return -1;
    }
}

static int handleInitMessage(FractalServerMessage *fmsg, size_t fmsg_size) {
    if (fmsg_size !=
        sizeof(FractalServerMessage) + sizeof(FractalServerMessageInit)) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: init message)!");
        return -1;
    }

    LOG_INFO("Received init message!\n");

    FractalServerMessageInit *fmsg_init =
        (FractalServerMessageInit *)fmsg->init_msg;

    memcpy(filename, fmsg_init->filename,
           min(sizeof(filename), sizeof(fmsg_init->filename)));
    memcpy(username, fmsg_init->username,
           min(sizeof(username), sizeof(fmsg_init->username)));

    if (logConnectionID(fmsg_init->connection_id) < 0) {
        LOG_ERROR("Failed to log connection ID.");
        return -1;
    }

    received_server_init_message = true;
    return 0;
}

static int handlePongMessage(FractalServerMessage *fmsg, size_t fmsg_size) {
    if (fmsg_size != sizeof(FractalServerMessage)) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: pong message)!");
        return -1;
    }
    if (ping_id == fmsg->ping_id) {
        LOG_INFO("Latency: %f", GetTimer(latency_timer));
        is_timing_latency = false;
        ping_failures = 0;
        try_amount = 0;
    } else {
        LOG_INFO("Old Ping ID found: Got %d but expected %d", fmsg->ping_id,
                 ping_id);
    }
    return 0;
}

static int handleQuitMessage(FractalServerMessage *fmsg, size_t fmsg_size) {
    fmsg;
    if (fmsg_size != sizeof(FractalServerMessage)) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: quit message)!");
        return -1;
    }
    LOG_INFO("Server signaled a quit!");
    exiting = true;
    return 0;
}

static int handleAudioFrequencyMessage(FractalServerMessage *fmsg,
                                       size_t fmsg_size) {
    if (fmsg_size != sizeof(FractalServerMessage)) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: audio frequency message)!");
        return -1;
    }
    LOG_INFO("Changing audio frequency to %d", fmsg->frequency);
    audio_frequency = fmsg->frequency;
    return 0;
}

static int handleClipboardMessage(FractalServerMessage *fmsg,
                                  size_t fmsg_size) {
    if (fmsg_size != sizeof(FractalServerMessage) + fmsg->clipboard.size) {
        LOG_ERROR(
            "Incorrect message size for a server message"
            " (type: clipboard message)!");
        return -1;
    }
    LOG_INFO("Received %d byte clipboard message from server!", fmsg_size);
    if (!ClipboardSynchronizerSetClipboard(&fmsg->clipboard)) {
        LOG_ERROR("Failed to set local clipboard from server message.");
        return -1;
    }
    return 0;
}
