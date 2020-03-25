#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <process.h>
#include <shlwapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
// TODO: Linux headers
#endif

#include "../include/fractal.h"
#ifdef _WIN32
#include "../include/wasapicapture.h"
#endif
#include "../include/videoencode.h"
#ifdef _WIN32
#include "../include/desktop.h"
#include "../include/dxgicapture.h"
#include "../include/input.h"
#endif
#include "../include/aes.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif
// Linux shouldn't have this

#define USE_GPU 0
#define USE_MONITOR 0
#define DEFAULT_WIDTH 1920
#define DEFAULT_HEIGHT 1080

static volatile bool connected;
static volatile double max_mbps;
static volatile int gop_size = 48;
#ifdef _WIN32
static volatile DesktopContext desktopContext = {0};
#endif
volatile int client_width = DEFAULT_WIDTH;
volatile int client_height = DEFAULT_HEIGHT;
volatile bool update_device = true;
volatile FractalCursorID last_cursor;

char buf[LARGEST_FRAME_SIZE];

#define VIDEO_BUFFER_SIZE 25
#define MAX_VIDEO_INDEX 500
struct RTPPacket video_buffer[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];
int video_buffer_packet_len[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];

#define AUDIO_BUFFER_SIZE 100
#define MAX_NUM_AUDIO_INDICES 3
struct RTPPacket audio_buffer[AUDIO_BUFFER_SIZE][MAX_NUM_AUDIO_INDICES];
int audio_buffer_packet_len[AUDIO_BUFFER_SIZE][MAX_NUM_AUDIO_INDICES];

SDL_mutex* packet_mutex;

struct SocketContext PacketSendContext = {0};

int ReplayPacket(struct SocketContext* context, struct RTPPacket* packet,
                 int len) {
    if (len > sizeof(struct RTPPacket)) {
        mprintf("Len too long!\n");
        return -1;
    }

    packet->is_a_nack = true;

    struct RTPPacket encrypted_packet;
    int encrypt_len = encrypt_packet(packet, len, &encrypted_packet,
                                     (unsigned char*)PRIVATE_KEY);

    SDL_LockMutex(packet_mutex);
    int sent_size = sendp(context, &encrypted_packet, encrypt_len);
    SDL_UnlockMutex(packet_mutex);

    if (sent_size < 0) {
        mprintf("Could not replay packet!\n");
        return -1;
    }

    return 0;
}

static char single_packet_buf[10000000];
static char encrypted_single_packet_buf[10000000];
int SendTCPPacket(struct SocketContext* context, FractalPacketType type,
                  uint8_t* data, int len) {
    struct RTPPacket* packet = (struct RTPPacket*)single_packet_buf;

    // Construct packet
    packet->type = type;
    memcpy(packet->data, data, len);
    packet->index = 0;
    packet->payload_size = len;
    packet->id = -1;
    packet->num_indices = 1;
    packet->is_a_nack = false;
    int packet_size = PACKET_HEADER_SIZE + packet->payload_size;

    // Encrypt the packet
    int encrypt_len = encrypt_packet(
        packet, packet_size,
        (struct RTPPacket*)(sizeof(int) + encrypted_single_packet_buf),
        (unsigned char*)PRIVATE_KEY);
    *((int*)encrypted_single_packet_buf) = encrypt_len;

    // Send it off
    mprintf("Sending %d bytes over TCP!\n", encrypt_len);
    int sent_size =
        sendp(context, encrypted_single_packet_buf, sizeof(int) + encrypt_len);

    if (sent_size < 0) {
#ifdef _WIN32
        int error = WSAGetLastError();
        mprintf("Unexpected Packet Error: %d\n", error);
#else
// TODO: Unix error handling
#endif
        return -1;
    }
    return 0;  // success
}

int SendPacket(struct SocketContext* context, FractalPacketType type,
               uint8_t* data, int len, int id) {
    if (id <= 0) {
        mprintf("IDs must be positive!\n");
        return -1;
    }

    int payload_size;
    int curr_index = 0, i = 0;

    clock packet_timer;
    StartTimer(&packet_timer);

    int num_indices =
        len / MAX_PAYLOAD_SIZE + (len % MAX_PAYLOAD_SIZE == 0 ? 0 : 1);

    double max_delay = 5.0;
    double delay_thusfar = 0.0;

    while (curr_index < len) {
        // Delay distribution of packets as needed
        if (((double)curr_index - 10000) / (len + 20000) * max_delay >
            delay_thusfar) {
            SDL_Delay(1);
            delay_thusfar += 1;
        }

        // local packet and len for when nack buffer isn't needed
        struct RTPPacket l_packet = {0};
        int l_len = 0;

        int* packet_len = &l_len;
        struct RTPPacket* packet = &l_packet;

        // Based on packet type, the packet to one of the buffers to serve later
        // nacks
        if (type == PACKET_AUDIO) {
            if (i >= MAX_NUM_AUDIO_INDICES) {
                mprintf("Audio index too long!\n");
                return -1;
            } else {
                packet = &audio_buffer[id % AUDIO_BUFFER_SIZE][i];
                packet_len =
                    &audio_buffer_packet_len[id % AUDIO_BUFFER_SIZE][i];
            }
        } else if (type == PACKET_VIDEO) {
            if (i >= MAX_VIDEO_INDEX) {
                mprintf("Video index too long!\n");
                return -1;
            } else {
                packet = &video_buffer[id % VIDEO_BUFFER_SIZE][i];
                packet_len =
                    &video_buffer_packet_len[id % VIDEO_BUFFER_SIZE][i];
            }
        }
        payload_size = min(MAX_PAYLOAD_SIZE, (len - curr_index));

        // Construct packet
        packet->type = type;
        memcpy(packet->data, data + curr_index, payload_size);
        packet->index = (short)i;
        packet->payload_size = payload_size;
        packet->id = id;
        packet->num_indices = (short)num_indices;
        packet->is_a_nack = false;
        int packet_size = PACKET_HEADER_SIZE + packet->payload_size;

        // Save the len to nack buffer lens
        *packet_len = packet_size;

        // Encrypt the packet
        struct RTPPacket encrypted_packet;
        int encrypt_len = encrypt_packet(packet, packet_size, &encrypted_packet,
                                         (unsigned char*)PRIVATE_KEY);

        // Send it off
        SDL_LockMutex(packet_mutex);
        int sent_size = sendp(context, &encrypted_packet, encrypt_len);
        SDL_UnlockMutex(packet_mutex);

        if (sent_size < 0) {
            int error = GetLastNetworkError();
            mprintf("Unexpected Packet Error: %d\n", error);
            return -1;
        }

        i++;
        curr_index += payload_size;
    }

    return 0;
}

static int32_t SendVideo(void* opaque) {
    SDL_Delay( 500 );

    struct SocketContext socketContext = *(struct SocketContext*)opaque;

    // Init DXGI Device
    struct CaptureDevice rdevice;
    struct CaptureDevice* device = NULL;

    struct FractalCursorTypes* types =
        (struct FractalCursorTypes*)malloc(sizeof(struct FractalCursorTypes));
    memset(types, 0, sizeof(struct FractalCursorTypes));
    LoadCursors(types);

    // Init FFMPEG Encoder
    int current_bitrate = STARTING_BITRATE;
    encoder_t* encoder = NULL;

    bool update_encoder = false;

    double worst_fps = 40.0;
    int ideal_bitrate = current_bitrate;
    int bitrate_tested_frames = 0;
    int bytes_tested_frames = 0;

    clock previous_frame_time;
    StartTimer(&previous_frame_time);
    int previous_frame_size = 0;

    int consecutive_capture_screen_errors = 0;

    //    int defaultCounts = 1;

    clock world_timer;
    StartTimer(&world_timer);

    int id = 1;
    int frames_since_first_iframe = 0;
    update_device = true;

    while (connected) {
        // Update device with new parameters
        if (update_device) {
            update_device = false;

            if (device) {
                DestroyCaptureDevice(device);
                device = NULL;
            }

            device = &rdevice;
            int result =
                CreateCaptureDevice(device, client_width, client_height);
            if (result < 0) {
                mprintf("Failed to create capture device\n");
                device = NULL;
                update_device = true;

                SDL_Delay(500);
                continue;
            }

            mprintf("Created Capture Device of dimensions %dx%d\n",
                    device->width, device->height);

            update_encoder = true;
        }

        // Update encoder with new parameters
        if (update_encoder) {
            if (encoder) {
                destroy_video_encoder(encoder);
            }
            encoder = create_video_encoder(device->width, device->height,
                                           current_bitrate, gop_size);
            update_encoder = false;
            frames_since_first_iframe = 0;
        }

        int accumulated_frames = CaptureScreen(device);

        // If capture screen failed, we should try again
        if (accumulated_frames < 0) {
            mprintf("Failed to capture screen\n");

            DestroyCaptureDevice(device);
            device = NULL;
            update_device = true;

            SDL_Delay(500);
            continue;
        }

        clock server_frame_timer;
        StartTimer(&server_frame_timer);

        // Only if we have a frame to render
        if (accumulated_frames > 0) {
            if (accumulated_frames > 1) {
                mprintf("Accumulated Frames: %d\n", accumulated_frames);
            }

            consecutive_capture_screen_errors = 0;

            clock t;
            StartTimer(&t);
            video_encoder_encode(encoder, device->frame_data);
            mprintf("Encode Time: %f\n", GetTimer(t));

            bitrate_tested_frames++;
            bytes_tested_frames += encoder->packet.size;

            if (encoder->packet.size != 0) {
                double delay = -1.0;

                if (previous_frame_size > 0) {
                    double frame_time = GetTimer(previous_frame_time);
                    StartTimer(&previous_frame_time);
                    // double mbps = previous_frame_size * 8.0 / 1024.0 / 1024.0
                    // / frame_time; TODO: bitrate throttling alg
                    // previousFrameSize * 8.0 / 1024.0 / 1024.0 / IdealTime =
                    // max_mbps previousFrameSize * 8.0 / 1024.0 / 1024.0 /
                    // max_mbps = IdealTime
                    double transmit_time =
                        previous_frame_size * 8.0 / 1024.0 / 1024.0 / max_mbps;

                    // double average_frame_size = 1.0 * bytes_tested_frames /
                    // bitrate_tested_frames;
                    double current_trasmit_time =
                        previous_frame_size * 8.0 / 1024.0 / 1024.0 / max_mbps;
                    double current_fps = 1.0 / current_trasmit_time;

                    delay = transmit_time - frame_time;
                    delay = min(delay, 0.004);

                    // mprintf("Size: %d, MBPS: %f, VS MAX MBPS: %f, Time: %f,
                    // Transmit Time: %f, Delay: %f\n", previous_frame_size,
                    // mbps, max_mbps, frame_time, transmit_time, delay);

                    if ((current_fps < worst_fps ||
                         ideal_bitrate > current_bitrate) &&
                        bitrate_tested_frames > 20) {
                        // Rather than having lower than the worst acceptable
                        // fps, find the ratio for what the bitrate should be
                        double ratio_bitrate = current_fps / worst_fps;
                        int new_bitrate =
                            (int)(ratio_bitrate * current_bitrate);
                        if (abs(new_bitrate - current_bitrate) / new_bitrate >
                            0.05) {
                            mprintf("Updating bitrate from %d to %d\n",
                                    current_bitrate, new_bitrate);
                            // TODO: Analyze bitrate handling with GOP size
                            // current_bitrate = new_bitrate;
                            // update_encoder = true;

                            bitrate_tested_frames = 0;
                            bytes_tested_frames = 0;
                        }
                    }
                }

                int frame_size = sizeof(Frame) + encoder->packet.size;
                if (frame_size > LARGEST_FRAME_SIZE) {
                    mprintf("Frame too large: %d\n", frame_size);
                } else {
                    // Create frame struct with compressed frame data and
                    // metadata
                    Frame* frame = (Frame*)buf;
                    frame->width = device->width;
                    frame->height = device->height;
                    frame->size = encoder->packet.size;
                    frame->cursor = GetCurrentCursor(types);
                    frame->is_iframe =
                        frames_since_first_iframe % gop_size == 0;
                    memcpy(frame->compressed_frame, encoder->packet.data,
                           encoder->packet.size);

                    // mprintf("Sent video packet %d (Size: %d) %s\n", id,
                    // encoder->packet.size, frame->is_iframe ? "(I-frame)" :
                    // "");

                    // Send video packet to client
                    if (SendPacket(&socketContext, PACKET_VIDEO,
                                   (uint8_t*)frame, frame_size, id) < 0) {
                        mprintf("Could not send video frame ID %d\n", id);
                    }
                    frames_since_first_iframe++;
                    id++;
                    previous_frame_size = encoder->packet.size;
                    // double server_frame_time = GetTimer(server_frame_timer);
                    // mprintf("Server Frame Time for ID %d: %f\n", id,
                    // server_frame_time);
                }
            }

            ReleaseScreen(device);
        }
    }
#ifdef _WIN32
    HCURSOR new_cursor = LoadCursor(NULL, IDC_ARROW);

    SetSystemCursor(new_cursor, last_cursor);
#else
// TODO: Linux cursor instead
#endif
    DestroyCaptureDevice(device);
    device = NULL;

    return 0;
}

#ifdef _WIN32
static int32_t SendAudio(void* opaque) {
    struct SocketContext context = *(struct SocketContext*)opaque;
    int id = 1;

    wasapi_device* audio_device =
        (wasapi_device*)malloc(sizeof(struct wasapi_device));
    audio_device = CreateAudioDevice(audio_device);
    StartAudioDevice(audio_device);

    // Tell the client what audio frequency we're using
    FractalServerMessage fmsg;
    fmsg.type = MESSAGE_AUDIO_FREQUENCY;
    fmsg.frequency = audio_device->pwfx->nSamplesPerSec;
    SendPacket(&PacketSendContext, PACKET_MESSAGE, (uint8_t*)&fmsg,
               sizeof(fmsg), 1);

    mprintf("Audio Frequency: %d\n", audio_device->pwfx->nSamplesPerSec);

    HRESULT hr = CoInitialize(NULL);
    DWORD dwWaitResult;
    UINT32 nNumFramesToRead, nNextPacketSize,
        nBlockAlign = audio_device->pwfx->nBlockAlign;
    DWORD dwFlags;

    while (connected) {
        for (hr = audio_device->pAudioCaptureClient->lpVtbl->GetNextPacketSize(
                 audio_device->pAudioCaptureClient, &nNextPacketSize);
             SUCCEEDED(hr) && nNextPacketSize > 0;
             hr = audio_device->pAudioCaptureClient->lpVtbl->GetNextPacketSize(
                 audio_device->pAudioCaptureClient, &nNextPacketSize)) {
            // Receive audio buffer
            audio_device->pAudioCaptureClient->lpVtbl->GetBuffer(
                audio_device->pAudioCaptureClient, &audio_device->pData,
                &nNumFramesToRead, &dwFlags, NULL, NULL);

            audio_device->audioBufSize = nNumFramesToRead * nBlockAlign;

            if (audio_device->audioBufSize > 10000) {
                mprintf("Audio buffer size too large!\n");
            } else if (audio_device->audioBufSize > 0) {
                // Send buffer
                if (SendPacket(&context, PACKET_AUDIO, audio_device->pData,
                               audio_device->audioBufSize, id) < 0) {
                    mprintf("Could not send audio frame\n");
                }
                id++;
            }

            // Free buffer
            audio_device->pAudioCaptureClient->lpVtbl->ReleaseBuffer(
                audio_device->pAudioCaptureClient, nNumFramesToRead);
        }
        dwWaitResult = WaitForSingleObject(audio_device->hWakeUp, INFINITE);
    }

    DestroyAudioDevice(audio_device);
    return 0;
}
#else
static int32_t SendAudio(void* opaque) {
    // TODO: The entire function must be remade, it relies heavily on windows
    // API
    return 0;
}
#endif

void update() {
    mprintf("Checking for updates...\n");
    runcmd(
#ifdef _WIN32
        "powershell -command \"iwr -outf 'C:\\Program "
        "Files\\Fractal\\update.bat' "
        "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/update.bat\""
#else
        "TODO: Linux command?"
#endif
    );
    runcmd(
#ifdef _WIN32
        "cmd.exe /C \"C:\\Program Files\\Fractal\\update.bat\""
#else
        "TODO: Linux command?"
#endif
    );
}

int main() {
    initMultiThreadedPrintf(true);
// TODO: Desktop operational for Unix as well
#ifdef _WIN32
    InitDesktop();
#endif

// initialize the windows socket library if this is a windows client
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        mprintf("Failed to initialize Winsock with error code: %d.\n",
                WSAGetLastError());
        return -1;
    }
#else
// TODO: Linux initialization
#endif

    while (true) {
        struct SocketContext PacketReceiveContext = {0};
        struct SocketContext PacketTCPContext = {0};

        if (CreateUDPContext(&PacketReceiveContext, "S", "0.0.0.0",
                             PORT_CLIENT_TO_SERVER, 1, 5000) < 0) {
            mprintf("Failed to start connection\n");

            // Since we're just idling, let's try updating the server
            update();

            continue;
        }

        if (CreateUDPContext(&PacketSendContext, "S", "0.0.0.0",
                             PORT_SERVER_TO_CLIENT, 1, 500) < 0) {
            mprintf(
                "Failed to finish connection (Failed at port server to "
                "client).\n");
            closesocket(PacketReceiveContext.s);
            continue;
        }

        if (CreateTCPContext(&PacketTCPContext, "S", "0.0.0.0", PORT_SHARED_TCP,
                             1, 500) < 0) {
            mprintf("Failed to finish connection (Failed at TCP context).\n");
            closesocket(PacketReceiveContext.s);
            closesocket(PacketSendContext.s);
            continue;
        }
// TODO: Have it for Linux
#ifdef _WIN32
        InitDesktop();
#endif
        // Give client time to setup before sending it with packets
        SDL_Delay(150);

        clock startup_time;
        StartTimer(&startup_time);

        connected = true;
        max_mbps = MAXIMUM_MBPS;

        packet_mutex = SDL_CreateMutex();

        SDL_Thread* send_video =
            SDL_CreateThread(SendVideo, "SendVideo", &PacketSendContext);
        SDL_Thread* send_audio =
            SDL_CreateThread(SendAudio, "SendAudio", &PacketSendContext);
        mprintf("Sending video and audio...\n");

        struct FractalClientMessage local_fmsg;
        struct FractalClientMessage* fmsg;

        clock last_ping;
        StartTimer(&last_ping);

        clock totaltime;
        StartTimer(&totaltime);

        clock last_exit_check;
        StartTimer(&last_exit_check);

        mprintf("Receiving packets...\n");

        int last_input_id = -1;
        StartTrackingClipboardUpdates();

        ClearReadingTCP();

        while (connected) {
            // If they clipboard as updated, we should send it over to the
            // client
            if (hasClipboardUpdated()) {
                FractalServerMessage* fmsg_response = malloc(10000000);
                fmsg_response->type = SMESSAGE_CLIPBOARD;
                ClipboardData* cb = GetClipboard();
                memcpy(&fmsg_response->clipboard, cb,
                       sizeof(ClipboardData) + cb->size);
                mprintf("Received clipboard trigger! Sending to client\n");
                if (SendTCPPacket(&PacketTCPContext, PACKET_MESSAGE,
                                  (uint8_t*)fmsg_response,
                                  sizeof(FractalServerMessage) + cb->size) <
                    0) {
                    mprintf("Could not send Clipboard Message\n");
                } else {
                    mprintf("Send clipboard message!\n");
                }
                free(fmsg_response);
            }

            if (GetTimer(last_ping) > 3.0) {
                mprintf("Client connection dropped.\n");
                connected = false;
            }

            if (GetTimer(last_exit_check) > 15.0 / 1000.0) {
// Exit file seen, time to exit
#ifdef _WIN32
                if (PathFileExistsA("C:\\Program Files\\Fractal\\Exit\\exit")) {
                    mprintf("Exiting due to button press...\n");
                    FractalServerMessage fmsg_response = {0};
                    fmsg_response.type = SMESSAGE_QUIT;
                    if (SendPacket(&PacketSendContext, PACKET_MESSAGE,
                                   (uint8_t*)&fmsg_response,
                                   sizeof(FractalServerMessage), 1) < 0) {
                        mprintf("Could not send Quit Message\n");
                    }
                    // Give a bit of time to make sure no one is touching it
                    SDL_Delay(50);
                    DeleteFileA("C:\\Program Files\\Fractal\\Exit\\exit");
                    connected = false;
                }
#else
// TODO: Filesystem for Unix
#endif
                StartTimer(&last_exit_check);
            }

            // START Get Packet
            char* tcp_buf = TryReadingTCPPacket(&PacketTCPContext);
            if (tcp_buf) {
                struct RTPPacket* packet = (struct RTPPacket*)tcp_buf;
                fmsg = (FractalClientMessage*)packet->data;
                mprintf("Received TCP BUF!!!! Size %d\n", packet->payload_size);
                mprintf("Received %d byte clipboard message from client.\n",
                        packet->payload_size);
            } else {
                memset(&local_fmsg, 0, sizeof(local_fmsg));

                fmsg = &local_fmsg;

                struct RTPPacket encrypted_packet;
                int encrypted_len;
                if ((encrypted_len =
                         recvp(&PacketReceiveContext, &encrypted_packet,
                               sizeof(encrypted_packet))) > 0) {
                    // Decrypt using AES private key
                    struct RTPPacket decrypted_packet;
                    int decrypt_len = decrypt_packet(
                        &encrypted_packet, encrypted_len, &decrypted_packet,
                        (unsigned char*)PRIVATE_KEY);

                    // If decrypted successfully
                    if (decrypt_len > 0) {
                        // Copy data into an fmsg
                        memcpy(
                            fmsg, decrypted_packet.data,
                            max(sizeof(*fmsg), decrypted_packet.payload_size));

                        // Check to see if decrypted packet is of valid size
                        if (decrypted_packet.payload_size !=
                            GetFmsgSize(fmsg)) {
                            mprintf("Packet is of the wrong size!: %d\n",
                                    decrypted_packet.payload_size);
                            mprintf("Type: %d\n", fmsg->type);
                            fmsg->type = 0;
                        }

                        // Make sure that keyboard events are played in order
                        if (fmsg->type == MESSAGE_KEYBOARD ||
                            fmsg->type == MESSAGE_KEYBOARD_STATE) {
                            // Check that id is in order
                            if (decrypted_packet.id > last_input_id) {
                                decrypted_packet.id = last_input_id;
                            } else {
                                // Received keyboard input out of order, just
                                // ignore
                                fmsg->type = 0;
                            }
                        }
                    }
                }
            }
            // END Get Packet

            if (fmsg->type != 0) {
                if (fmsg->type == MESSAGE_KEYBOARD ||
                    fmsg->type == MESSAGE_MOUSE_BUTTON ||
                    fmsg->type == MESSAGE_MOUSE_WHEEL ||
                    fmsg->type == MESSAGE_MOUSE_MOTION) {
// TODO: Unix version missing
// Replay user input (keyboard or mouse presses)
#ifdef _WIN32
                    ReplayUserInput(fmsg);
#endif
                } else if (fmsg->type == MESSAGE_KEYBOARD_STATE) {
// TODO: Unix version missing
// Synchronize client and server keyboard state
#ifdef _WIN32
                    updateKeyboardState(fmsg);
#endif
                } else if (fmsg->type == MESSAGE_MBPS) {
                    // Update mbps
                    max_mbps = fmsg->mbps;
                } else if (fmsg->type == MESSAGE_PING) {
                    mprintf("Ping Received - ID %d\n", fmsg->ping_id);

                    // Response to ping with pong
                    FractalServerMessage fmsg_response = {0};
                    fmsg_response.type = MESSAGE_PONG;
                    fmsg_response.ping_id = fmsg->ping_id;
                    StartTimer(&last_ping);
                    if (SendPacket(&PacketSendContext, PACKET_MESSAGE,
                                   (uint8_t*)&fmsg_response,
                                   sizeof(fmsg_response), 1) < 0) {
                        mprintf("Could not send Pong\n");
                    }
                } else if (fmsg->type == MESSAGE_DIMENSIONS) {
                    mprintf("Request to use dimensions %dx%d received\n",
                            fmsg->dimensions.width, fmsg->dimensions.height);
                    // Update knowledge of client monitor dimensions
                    if (client_width != fmsg->dimensions.width ||
                        client_height != fmsg->dimensions.height) {
                        client_width = fmsg->dimensions.width;
                        client_height = fmsg->dimensions.height;
                        // Update device if knowledge changed
                        update_device = true;
                    }
                } else if (fmsg->type == CMESSAGE_CLIPBOARD) {
                    // Update clipboard with message
                    mprintf("Clipboard! %d\n", fmsg->clipboard.type);
                    SetClipboard(&fmsg->clipboard);
                } else if (fmsg->type == MESSAGE_AUDIO_NACK) {
                    // Audio nack received, relay the packet

                    // mprintf("Audio NACK requested for: ID %d Index %d\n",
                    // fmsg->nack_data.id, fmsg->nack_data.index);
                    struct RTPPacket* audio_packet =
                        &audio_buffer[fmsg->nack_data.id % AUDIO_BUFFER_SIZE]
                                     [fmsg->nack_data.index];
                    int len = audio_buffer_packet_len[fmsg->nack_data.id %
                                                      AUDIO_BUFFER_SIZE]
                                                     [fmsg->nack_data.index];
                    if (audio_packet->id == fmsg->nack_data.id) {
                        mprintf(
                            "NACKed audio packet %d found of length %d. "
                            "Relaying!\n",
                            fmsg->nack_data.id, len);
                        ReplayPacket(&PacketSendContext, audio_packet, len);
                    }
                    // If we were asked for an invalid index, just ignore it
                    else if (fmsg->nack_data.index <
                             audio_packet->num_indices) {
                        mprintf(
                            "NACKed audio packet %d %d not found, ID %d %d was "
                            "located instead.\n",
                            fmsg->nack_data.id, fmsg->nack_data.index,
                            audio_packet->id, audio_packet->index);
                    }
                } else if (fmsg->type == MESSAGE_VIDEO_NACK) {
                    // Video nack received, relay the packet

                    // mprintf("Video NACK requested for: ID %d Index %d\n",
                    // fmsg->nack_data.id, fmsg->nack_data.index);
                    struct RTPPacket* video_packet =
                        &video_buffer[fmsg->nack_data.id % VIDEO_BUFFER_SIZE]
                                     [fmsg->nack_data.index];
                    int len = video_buffer_packet_len[fmsg->nack_data.id %
                                                      VIDEO_BUFFER_SIZE]
                                                     [fmsg->nack_data.index];
                    if (video_packet->id == fmsg->nack_data.id) {
                        mprintf(
                            "NACKed video packet ID %d Index %d found of "
                            "length %d. Relaying!\n",
                            fmsg->nack_data.id, fmsg->nack_data.index, len);
                        ReplayPacket(&PacketSendContext, video_packet, len);
                    }

                    // If we were asked for an invalid index, just ignore it
                    else if (fmsg->nack_data.index <
                             video_packet->num_indices) {
                        mprintf(
                            "NACKed video packet %d %d not found, ID %d %d was "
                            "located instead.\n",
                            fmsg->nack_data.id, fmsg->nack_data.index,
                            video_packet->id, video_packet->index);
                    }
                } else if (fmsg->type == CMESSAGE_QUIT) {
                    // Client requested to exit, it's time to disconnect
                    mprintf("Client Quit\n");
                    connected = false;
                }
            }
        }

        mprintf("Disconnected\n");

        SDL_WaitThread(send_video, NULL);
        SDL_WaitThread(send_audio, NULL);

        SDL_DestroyMutex(packet_mutex);

        closesocket(PacketReceiveContext.s);
        closesocket(PacketSendContext.s);
        closesocket(PacketTCPContext.s);
    }

#ifdef _WIN32
    WSACleanup();
#endif

    destroyMultiThreadedPrintf();

    return 0;
}
