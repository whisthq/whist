#include "../fractal/core/fractal.h"

#include "../fractal/clipboard/clipboard.h"
#include "../fractal/input/input.h"
#include "../fractal/network/network.h"
#include "../fractal/utils/clock.h"
#include "../fractal/utils/logging.h"
#include "client.h"
#include "handle_client_message.h"
#include "network.h"

#ifdef _WIN32
#include "../fractal/utils/windows_utils.h"
#endif

extern Client clients[MAX_NUM_CLIENTS];

#define VIDEO_BUFFER_SIZE 25
#define MAX_VIDEO_INDEX 500
extern FractalPacket video_buffer[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];
extern int video_buffer_packet_len[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];

#define AUDIO_BUFFER_SIZE 100
#define MAX_NUM_AUDIO_INDICES 3
extern FractalPacket audio_buffer[AUDIO_BUFFER_SIZE][MAX_NUM_AUDIO_INDICES];
extern int audio_buffer_packet_len[AUDIO_BUFFER_SIZE][MAX_NUM_AUDIO_INDICES];

extern volatile double max_mbps;
extern volatile int client_width;
extern volatile int client_height;
extern volatile bool update_device;
extern volatile CodecType client_codec_type;

extern volatile bool wants_iframe;
extern volatile bool update_encoder;
extern input_device_t *input_device;

static int handleUserInputMessage(FractalClientMessage *fmsg, int client_id,
                                  bool is_controlling);
static int handleKeyboardStateMessage(FractalClientMessage *fmsg, int client_id,
                                      bool is_controlling);
static int handleBitrateMessage(FractalClientMessage *fmsg, int client_id,
                                bool is_controlling);
static int handlePingMessage(FractalClientMessage *fmsg, int client_id,
                             bool is_controlling);
static int handleDimensionsMessage(FractalClientMessage *fmsg, int client_id,
                                   bool is_controlling);
static int handleClipboardMessage(FractalClientMessage *fmsg, int client_id,
                                  bool is_controlling);
static int handleAudioNackMessage(FractalClientMessage *fmsg, int client_id,
                                  bool is_controlling);
static int handleVideoNackMessage(FractalClientMessage *fmsg, int client_id,
                                  bool is_controlling);
static int handleIFrameRequestMessage(FractalClientMessage *fmsg, int client_id,
                                      bool is_controlling);
static int handleInteractionModeMessage(FractalClientMessage *fmsg,
                                        int client_id, bool is_controlling);
static int handleQuitMessage(FractalClientMessage *fmsg, int client_id,
                             bool is_controlling);
static int handleTimeMessage(FractalClientMessage *fmsg, int client_id,
                             bool is_controlling);
static int handleMouseInactiveMessage(FractalClientMessage *fmsg, int client_id,
                                      bool is_controlling);

int handleClientMessage(FractalClientMessage *fmsg, int client_id,
                        bool is_controlling) {
    switch (fmsg->type) {
        case MESSAGE_KEYBOARD:
        case MESSAGE_MOUSE_BUTTON:
        case MESSAGE_MOUSE_WHEEL:
        case MESSAGE_MOUSE_MOTION:
            return handleUserInputMessage(fmsg, client_id, is_controlling);
        case MESSAGE_KEYBOARD_STATE:
            return handleKeyboardStateMessage(fmsg, client_id, is_controlling);
        case MESSAGE_MBPS:
            return handleBitrateMessage(fmsg, client_id, is_controlling);
        case MESSAGE_PING:
            return handlePingMessage(fmsg, client_id, is_controlling);
        case MESSAGE_DIMENSIONS:
            return handleDimensionsMessage(fmsg, client_id, is_controlling);
        case CMESSAGE_CLIPBOARD:
            return handleClipboardMessage(fmsg, client_id, is_controlling);
        case MESSAGE_AUDIO_NACK:
            return handleAudioNackMessage(fmsg, client_id, is_controlling);
        case MESSAGE_VIDEO_NACK:
            return handleVideoNackMessage(fmsg, client_id, is_controlling);
        case MESSAGE_IFRAME_REQUEST:
            return handleIFrameRequestMessage(fmsg, client_id, is_controlling);
        case CMESSAGE_INTERACTION_MODE:
            return handleInteractionModeMessage(fmsg, client_id,
                                                is_controlling);
        case CMESSAGE_QUIT:
            return handleQuitMessage(fmsg, client_id, is_controlling);
        case MESSAGE_TIME:
            return handleTimeMessage(fmsg, client_id, is_controlling);
        case MESSAGE_MOUSE_INACTIVE:
            return handleMouseInactiveMessage(fmsg, client_id, is_controlling);
        default:
            LOG_WARNING(
                "Unknown FractalClientMessage Received. "
                "(Type: %d)",
                fmsg->type);
            return -1;
    }
}

// is called with is active read locked
static int handleUserInputMessage(FractalClientMessage *fmsg, int client_id,
                                  bool is_controlling) {
    // Replay user input (keyboard or mouse presses)
    if (is_controlling && input_device) {
        if (!ReplayUserInput(input_device, fmsg)) {
            LOG_WARNING("Failed to replay input!");
#ifdef _WIN32
            InitDesktop();
#endif
        }
    }

    if (fmsg->type == MESSAGE_MOUSE_MOTION) {
        if (SDL_LockMutex(state_lock) != 0) {
            LOG_ERROR("Failed to unlock client's mouse lock.");
            return -1;
        }
        clients[client_id].mouse.is_active = true;
        clients[client_id].mouse.x = fmsg->mouseMotion.x_nonrel;
        clients[client_id].mouse.y = fmsg->mouseMotion.y_nonrel;
        SDL_UnlockMutex(state_lock);
    }

    return 0;
}

// TODO: Unix version missing
// Synchronize client and server keyboard state
static int handleKeyboardStateMessage(FractalClientMessage *fmsg, int client_id,
                                      bool is_controlling) {
    client_id;
    if (!is_controlling) return 0;
#ifdef _WIN32
    UpdateKeyboardState(input_device, fmsg);
#endif
    return 0;
}

// Update mbps
// idk how to handle this
static int handleBitrateMessage(FractalClientMessage *fmsg, int client_id,
                                bool is_controlling) {
    client_id;
    if (!is_controlling) return 0;
    LOG_INFO("MSG RECEIVED FOR MBPS: %f\n", fmsg->mbps);
    max_mbps = max(fmsg->mbps, MINIMUM_BITRATE / 1024.0 / 1024.0);
    // update_encoder = true;
    return 0;
}

static int handlePingMessage(FractalClientMessage *fmsg, int client_id,
                             bool is_controlling) {
    is_controlling;
    LOG_INFO("Ping Received - Client ID: %d, Ping ID %d", client_id,
             fmsg->ping_id);

    // Update ping timer
    StartTimer(&(clients[client_id].last_ping));

    // Send pong reply
    FractalServerMessage fmsg_response = {0};
    fmsg_response.type = MESSAGE_PONG;
    fmsg_response.ping_id = fmsg->ping_id;
    int ret = 0;
    if (SendUDPPacket(&(clients[client_id].UDP_context), PACKET_MESSAGE,
                      (uint8_t *)&fmsg_response, sizeof(fmsg_response), 1,
                      STARTING_BURST_BITRATE, NULL, NULL) < 0) {
        LOG_WARNING("Could not send Ping to Client ID: %d", client_id);
        ret = -1;
    }

    return ret;
}

static int handleDimensionsMessage(FractalClientMessage *fmsg, int client_id,
                                   bool is_controlling) {
    client_id;
    if (!is_controlling) return 0;
    // Update knowledge of client monitor dimensions
    LOG_INFO("Request to use dimensions %dx%d received", fmsg->dimensions.width,
             fmsg->dimensions.height);
    if (client_width != fmsg->dimensions.width ||
        client_height != fmsg->dimensions.height ||
        client_codec_type != fmsg->dimensions.codec_type) {
        client_width = fmsg->dimensions.width;
        client_height = fmsg->dimensions.height;
        client_codec_type = fmsg->dimensions.codec_type;
        // Update device if knowledge changed
        update_device = true;
    }
    return 0;
}

static int handleClipboardMessage(FractalClientMessage *fmsg, int client_id,
                                  bool is_controlling) {
    client_id;
    if (!is_controlling) return 0;
    // Update clipboard with message
    LOG_INFO("Received Clipboard Data! %d", fmsg->clipboard.type);
    SetClipboard(&fmsg->clipboard);
    return 0;
}

// Audio nack received, relay the packet
static int handleAudioNackMessage(FractalClientMessage *fmsg, int client_id,
                                  bool is_controlling) {
    if (!is_controlling) return 0;
    // mprintf("Audio NACK requested for: ID %d Index %d\n",
    // fmsg->nack_data.id, fmsg->nack_data.index);
    FractalPacket *audio_packet =
        &audio_buffer[fmsg->nack_data.id % AUDIO_BUFFER_SIZE]
                     [fmsg->nack_data.index];
    int len = audio_buffer_packet_len[fmsg->nack_data.id % AUDIO_BUFFER_SIZE]
                                     [fmsg->nack_data.index];
    if (audio_packet->id == fmsg->nack_data.id) {
        LOG_INFO(
            "NACKed audio packet %d found of length %d. "
            "Relaying!",
            fmsg->nack_data.id, len);
        ReplayPacket(&(clients[client_id].UDP_context), audio_packet, len);
    }
    // If we were asked for an invalid index, just ignore it
    else if (fmsg->nack_data.index < audio_packet->num_indices) {
        LOG_WARNING(
            "NACKed audio packet %d %d not found, ID %d %d was "
            "located instead.",
            fmsg->nack_data.id, fmsg->nack_data.index, audio_packet->id,
            audio_packet->index);
    }
    return 0;
}
static int handleVideoNackMessage(FractalClientMessage *fmsg, int client_id,
                                  bool is_controlling) {
    if (!is_controlling) return 0;
    // Video nack received, relay the packet

    // mprintf("Video NACK requested for: ID %d Index %d\n",
    // fmsg->nack_data.id, fmsg->nack_data.index);
    FractalPacket *video_packet =
        &video_buffer[fmsg->nack_data.id % VIDEO_BUFFER_SIZE]
                     [fmsg->nack_data.index];
    int len = video_buffer_packet_len[fmsg->nack_data.id % VIDEO_BUFFER_SIZE]
                                     [fmsg->nack_data.index];
    if (video_packet->id == fmsg->nack_data.id) {
        LOG_INFO(
            "NACKed video packet ID %d Index %d found of "
            "length %d. Relaying!",
            fmsg->nack_data.id, fmsg->nack_data.index, len);
        ReplayPacket(&(clients[client_id].UDP_context), video_packet, len);
    }

    // If we were asked for an invalid index, just ignore it
    else if (fmsg->nack_data.index < video_packet->num_indices) {
        LOG_WARNING(
            "NACKed video packet %d %d not found, ID %d %d was "
            "located instead.",
            fmsg->nack_data.id, fmsg->nack_data.index, video_packet->id,
            video_packet->index);
    }
    return 0;
}

static int handleIFrameRequestMessage(FractalClientMessage *fmsg, int client_id,
                                      bool is_controlling) {
    client_id;
    is_controlling;
    LOG_INFO("Request for i-frame found: Creating iframe");
    if (fmsg->reinitialize_encoder) {
        update_encoder = true;
    } else {
        wants_iframe = true;
    }
    return 0;
}

static int handleInteractionModeMessage(FractalClientMessage *fmsg,
                                        int client_id, bool is_controlling) {
    /* IGNORE INTERACTION MODE MESSAGES */
    // return 0;

    is_controlling;
    if (SDL_LockMutex(state_lock) != 0) {
        LOG_ERROR("Failed to lock client's mouse lock.");
        return -1;
    }
    InteractionMode mode = fmsg->interaction_mode;
    if (mode == CONTROL || mode == EXCLUSIVE_CONTROL) {
        if (!clients[client_id].is_controlling) {
            clients[client_id].is_controlling = true;
            num_controlling_clients++;
        }
    } else if (mode == SPECTATE) {
        if (clients[client_id].is_controlling) {
            clients[client_id].is_controlling = false;
            num_controlling_clients--;
            if (num_controlling_clients == 0 && host_id != -1 &&
                clients[host_id].is_active) {
                clients[host_id].is_controlling = true;
            }
        }
    } else {
        LOG_ERROR("Unrecognized interaction mode (Mode: %d)", (int)mode);
        SDL_UnlockMutex(state_lock);
        return -1;
    }

    if (mode == EXCLUSIVE_CONTROL) {
        for (int id = 0; id < MAX_NUM_CLIENTS; id++) {
            if (clients[id].is_active && clients[id].is_controlling &&
                id != client_id) {
                clients[id].is_controlling = false;
            }
        }
    }
    SDL_UnlockMutex(state_lock);

    return 0;
}

static int handleQuitMessage(FractalClientMessage *fmsg, int client_id,
                             bool is_controlling) {
    is_controlling;
    fmsg;
    int ret = 0;
    if (readUnlock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to read unlock is active lock.");
        ret = -1;
    }
    if (writeLock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to write-acquire is active RW lock.");
        if (readLock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to read lock is active lock.");
            LOG_ERROR("BAD. IRRECOVERABLE.");
        }
        return -1;
    }
    if (SDL_LockMutex(state_lock) != 0) {
        LOG_ERROR("Failed to lock state lock");
        if (writeUnlock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to write-release is active RW lock.");
        }
        if (readLock(&is_active_rwlock) != 0) {
            LOG_ERROR("Failed to read lock is active lock.");
            LOG_ERROR("BAD. IRRECOVERABLE.");
        }
        return -1;
    }
    if (quitClient(client_id) != 0) {
        LOG_ERROR("Failed to quit client. (ID: %d)", client_id);
        ret = -1;
    }
    if (SDL_UnlockMutex(state_lock) != 0) {
        LOG_ERROR("Failed to unlock state lock");
        ret = -1;
    }
    if (writeUnlock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to write-release is active RW lock.");
        ret = -1;
    }
    if (readLock(&is_active_rwlock) != 0) {
        LOG_ERROR("Failed to read lock is active lock.");
        LOG_ERROR("BAD. IRRECOVERABLE.");
        ret = -1;
    }
    if (ret == 0) LOG_INFO("Client successfully quit. (ID: %d)", client_id);
    return ret;
}

static int handleTimeMessage(FractalClientMessage *fmsg, int client_id,
                             bool is_controlling) {
    client_id;
    if (!is_controlling) return 0;
    LOG_INFO("Recieving a message time packet");
#ifdef _WIN32
    if (fmsg->time_data.use_win_name) {
        LOG_INFO("Setting time from windows time zone %s",
                 fmsg->time_data.win_tz_name);
        SetTimezoneFromWindowsName(fmsg->time_data.win_tz_name);
    } else {
        LOG_INFO("Setting time from UTC offset %d", fmsg->time_data.UTC_Offset);
        SetTimezoneFromUtc(fmsg->time_data.UTC_Offset,
                           fmsg->time_data.DST_flag);
    }
#else
    if (fmsg->time_data.use_linux_name) {
        LOG_INFO("Setting time from IANA time zone %s",
                 fmsg->time_data.win_tz_name);
        SetTimezoneFromIANAName(fmsg->time_data.win_tz_name);
    } else {
        LOG_INFO("Setting time from UTC offset %d",
                 fmsg->time_data.win_tz_name);
        SetTimezoneFromUtc(fmsg->time_data.UTC_Offset,
                           fmsg->time_data.DST_flag);
    }
#endif
    return 0;
}

static int handleMouseInactiveMessage(FractalClientMessage *fmsg, int client_id,
                                      bool is_controlling) {
    fmsg;
    is_controlling;
    clients[client_id].mouse.is_active = false;
    return 0;
}
