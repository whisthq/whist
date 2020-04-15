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
#include <unistd.h>
// TODO: Linux headers
#endif

#include "../include/audiocapture.h"
#include "../include/audioencode.h"
#include "../include/fractal.h"
#include "../include/input.h"
#include "../include/screencapture.h"
#include "../include/videoencode.h"

#ifdef _WIN32
#include "../include/desktop.h"
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
static volatile int gop_size = 9999;
volatile int client_width = -1;
volatile int client_height = -1;
volatile bool update_device = true;
volatile FractalCursorID last_cursor;
// volatile

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

volatile bool wants_iframe;
volatile bool update_encoder;

#ifndef _WIN32
void InitCursors() { return; }

FractalCursorImage GetCurrentCursor() {
    FractalCursorImage image = {0};
    image.cursor_state = CURSOR_STATE_VISIBLE;
    image.cursor_id = SDL_SYSTEM_CURSOR_ARROW;
    return image;
}
#endif

int ReplayPacket(struct SocketContext* context, struct RTPPacket* packet,
                 size_t len) {
    if (len > sizeof(struct RTPPacket)) {
        mprintf("Len too long!\n");
        return -1;
    }

    packet->is_a_nack = true;

    struct RTPPacket encrypted_packet;
    int encrypt_len = encrypt_packet(packet, (int)len, &encrypted_packet,
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
        int error = GetLastNetworkError();
        mprintf("Unexpected Packet Error: %d\n", error);
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

    // double max_delay = 5.0;
    // double delay_thusfar = 0.0;

    // Send some amount of packets every two milliseconds
    int break_resolution = 2;

    double num_indices_per_unit_latency = (AVERAGE_LATENCY_MS / 1000.0) *
                                          (STARTING_BURST_BITRATE / 8.0) /
                                          MAX_PAYLOAD_SIZE;

    double break_distance = num_indices_per_unit_latency *
                            (1.0 * break_resolution / AVERAGE_LATENCY_MS);

    int num_breaks = (int)(num_indices / break_distance);
    if (num_breaks < 0) {
        num_breaks = 0;
    }
    int break_point = num_indices / (num_breaks + 1);

    /*
    if (type == PACKET_AUDIO) {
        static int ddata = 0;
        static clock last_timer;
        if( ddata == 0 )
        {
            StartTimer( &last_timer );
        }
        ddata += len;
        GetTimer( last_timer );
        if( GetTimer( last_timer ) > 5.0 )
        {
            mprintf( "AUDIO BANDWIDTH: %f kbps", 8 * ddata / GetTimer(
    last_timer ) / 1024 ); ddata = 0;
        }
        // mprintf("Video ID %d (Packets: %d)\n", id, num_indices);
    }
    */

    while (curr_index < len) {
        // Delay distribution of packets as needed
        if (i > 0 && break_point > 0 && i % break_point == 0 &&
            i < num_indices - break_point / 2) {
            SDL_Delay(break_resolution);
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

        // Encrypt the packet with AES
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
    SDL_Delay(500);

    struct SocketContext socketContext = *(struct SocketContext*)opaque;

    // Init DXGI Device
    struct CaptureDevice rdevice;
    struct CaptureDevice* device = NULL;

    InitCursors();

    // Init FFMPEG Encoder
    int current_bitrate = STARTING_BITRATE;
    encoder_t* encoder = NULL;

    double worst_fps = 40.0;
    int ideal_bitrate = current_bitrate;
    int bitrate_tested_frames = 0;
    int bytes_tested_frames = 0;

    clock previous_frame_time;
    StartTimer(&previous_frame_time);
    int previous_frame_size = 0;

    // int consecutive_capture_screen_errors = 0;

    //    int defaultCounts = 1;

    clock world_timer;
    StartTimer(&world_timer);

    int id = 1;
    int frames_since_first_iframe = 0;
    update_device = true;

    static clock last_frame_capture;
    StartTimer(&last_frame_capture);

    while (connected) {
        if (client_width < 0 || client_height < 0) {
            SDL_Delay(5);
            continue;
        }

        // Update device with new parameters
        if (update_device) {
            update_device = false;

            if (device) {
                DestroyCaptureDevice(device);
                device = NULL;
            }

            device = &rdevice;
            if (CreateCaptureDevice(device, client_width, client_height) < 0) {
                mprintf("Failed to create capture device\n");
                device = NULL;
                update_device = true;

                SDL_Delay(100);
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

        // Accumulated_frames is equal to how many frames have passed since the last call to CaptureScreen
        int accumulated_frames = 0;
        if (GetTimer(last_frame_capture) > 1.0 / FPS) {
            accumulated_frames = CaptureScreen(device);
        }

        // If capture screen failed, we should try again
        if (accumulated_frames < 0) {
            mprintf("Failed to capture screen\n");

            DestroyCaptureDevice(device);
            device = NULL;
            update_device = true;

            SDL_Delay(100);
            continue;
        }

        clock server_frame_timer;
        StartTimer(&server_frame_timer);

        // Only if we have a frame to render
        if (accumulated_frames > 0 || wants_iframe ||
            GetTimer(last_frame_capture) > 1.0 / MIN_FPS) {
            StartTimer(&last_frame_capture);

            if (accumulated_frames > 1) {
                mprintf("Accumulated Frames: %d\n", accumulated_frames);
            }
            if (accumulated_frames == 0) {
                mprintf("Sending current frame!\n");
            }

            // consecutive_capture_screen_errors = 0;

            bool is_iframe = false;
            if (wants_iframe) {
                video_encoder_set_iframe(encoder);
                wants_iframe = false;
                is_iframe = true;
            }

            clock t;
            StartTimer(&t);

            video_encoder_encode(encoder, device->frame_data);
            frames_since_first_iframe++;

            video_encoder_unset_iframe(encoder);

            // mprintf("Encode Time: %f (%d) (%d)\n", GetTimer(t),
            //        frames_since_first_iframe % gop_size,
            //        encoder->packet.size);

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
                    frame->cursor = GetCurrentCursor();
                    // True if this frame does not require previous frames to render
                    frame->is_iframe =
                        (frames_since_first_iframe % gop_size == 1) ||
                        is_iframe;
                    memcpy(frame->compressed_frame, encoder->packet.data,
                           encoder->packet.size);

                    // mprintf("Sent video packet %d (Size: %d) %s\n", id,
                    // encoder->packet.size, frame->is_iframe ? "(I-frame)" :
                    // "");

                    // Send video packet to client
                    if (SendPacket(&socketContext, PACKET_VIDEO,
                                   (uint8_t*)frame, frame_size, id) < 0) {
                        mprintf("Could not send video frame ID %d\n", id);
                    } else {
                        // Only increment ID if the send succeeded
                        id++;
                    }
                    previous_frame_size = encoder->packet.size;
                    // double server_frame_time = GetTimer(server_frame_timer);
                    // mprintf("Server Frame Time for ID %d: %f\n", id,
                    // server_frame_time);
                }
            } else {
                mprintf("Empty encoder packet");
            }
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

static int32_t SendAudio(void* opaque) {
    struct SocketContext context = *(struct SocketContext*)opaque;
    int id = 1;

    audio_device_t* audio_device =
        (audio_device_t*)malloc(sizeof(struct audio_device_t));
    audio_device = CreateAudioDevice(audio_device);
    mprintf("Created audio device!\n");
    if (!audio_device) {
        mprintf("Failed to create audio device...\n");
        return -1;
    }
    StartAudioDevice(audio_device);
    audio_encoder_t* audio_encoder =
        create_audio_encoder(AUDIO_BITRATE, audio_device->sample_rate);
    int res;

    // Tell the client what audio frequency we're using

    FractalServerMessage fmsg;
    fmsg.type = MESSAGE_AUDIO_FREQUENCY;
    fmsg.frequency = audio_device->sample_rate;
    SendPacket(&PacketSendContext, PACKET_MESSAGE, (uint8_t*)&fmsg,
               sizeof(fmsg), 1);
    mprintf("Audio Frequency: %d\n", audio_device->sample_rate);

    // setup

    while (connected) {
        // for each available packet
        for (GetNextPacket(audio_device); PacketAvailable(audio_device);
             GetNextPacket(audio_device)) {
            GetBuffer(audio_device);

            if (audio_device->buffer_size > 10000) {
                mprintf("Audio buffer size too large!\n");
            } else if (audio_device->buffer_size > 0) {
#if USING_AUDIO_ENCODE_DECODE

                // add samples to encoder fifo

                audio_encoder_fifo_intake(audio_encoder, audio_device->buffer,
                                          audio_device->frames_available);

                // while fifo has enough samples for an aac frame, handle it
                while (av_audio_fifo_size(audio_encoder->pFifo) >=
                       audio_encoder->pCodecCtx->frame_size) {
                    // create and encode a frame

                    res = audio_encoder_encode_frame(audio_encoder);

                    if (res < 0) {
                        // bad boy error
                        mprintf("error encoding packet\n");
                        continue;
                    } else if (res > 0) {
                        // no data or need more data
                        break;
                    }

                    // mprintf("we got a packet of size %d\n",
                    //         audio_encoder->packet.size);

                    // Send packet

                    if (SendPacket(&context, PACKET_AUDIO,
                                   audio_encoder->packet.data,
                                   audio_encoder->packet.size, id) < 0) {
                        mprintf("Could not send audio frame\n");
                    }
                    // mprintf("sent audio frame %d\n", id);
                    id++;

                    // Free encoder packet

                    av_packet_unref(&audio_encoder->packet);
                }
#else
                if (SendPacket(&context, PACKET_AUDIO, audio_device->buffer,
                               audio_device->buffer_size, id) < 0) {
                    mprintf("Could not send audio frame\n");
                }
                id++;
#endif
            }

            ReleaseBuffer(audio_device);
        }
        WaitTimer(audio_device);
    }

    // destroy_audio_encoder(audio_encoder);
    DestroyAudioDevice(audio_device);
    return 0;
}

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
    initBacktraceHandler();
    initMultiThreadedPrintf(true);
    initClipboard();
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_VIDEO);
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
#endif
    static_assert(sizeof(unsigned short) == 2,
                  "Error: Unsigned short is not length 2 bytes!\n");

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

        // Give client time to setup before sending it with packets
        SDL_Delay(150);

        FractalServerMessage* msg_init_whole = malloc(sizeof( FractalServerMessage) + sizeof( FractalServerMessageInit));
        msg_init_whole->type = MESSAGE_INIT;
        FractalServerMessageInit* msg_init = msg_init_whole->init_msg;
#ifdef _WIN32
	    msg_init->filename[0] = '\0';
        strcat( msg_init->filename, "C:\\Program Files\\Fractal" );
	    char* username = "vm1";
#else // Linux
        char* cwd = getcwd(NULL, 0);
        memcpy( msg_init->filename, cwd, strlen(cwd) + 1);
        free( cwd );
	    char* username = "Fractal";
#endif
        memcpy( msg_init->username, username, strlen(username) + 1 );
	    mprintf("SIZE: %d\n", sizeof(FractalServerMessage) + sizeof(FractalServerMessageInit));
        if( SendPacket( &PacketSendContext, PACKET_MESSAGE,
            (uint8_t*)msg_init_whole,
                           sizeof( FractalServerMessage ) + sizeof( FractalServerMessageInit ), 1 ) < 0 )
        {
            mprintf( "Could not send server init message!\n" );
            return -1;
        }
	    free(msg_init_whole);

        clock startup_time;
        StartTimer(&startup_time);

        connected = true;
        max_mbps = MAXIMUM_MBPS;
        wants_iframe = false;
        update_encoder = false;
        packet_mutex = SDL_CreateMutex();

        SDL_Thread* send_video =
            SDL_CreateThread(SendVideo, "SendVideo", &PacketSendContext);
        SDL_Thread* send_audio =
            SDL_CreateThread(SendAudio, "SendAudio", &PacketSendContext);
        mprintf("Sending video and audio...\n");

        input_device_t* input_device =
            (input_device_t*)malloc(sizeof(input_device_t));
        input_device = CreateInputDevice(input_device);
        if (!input_device) {
            mprintf("Failed to create input device for playback.\n");
        }

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
                        memcpy(fmsg, decrypted_packet.data,
                               max(sizeof(*fmsg),
                                   (size_t)decrypted_packet.payload_size));

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
                    // Replay user input (keyboard or mouse presses)
                    if (input_device) {
                        ReplayUserInput(input_device, fmsg);
                    }

                } else if (fmsg->type == MESSAGE_KEYBOARD_STATE) {
// TODO: Unix version missing
// Synchronize client and server keyboard state
#ifdef _WIN32
                    UpdateKeyboardState(input_device, fmsg);
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
                    mprintf("Received Clipboard Data! %d\n", fmsg->clipboard.type);
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
                } else if (fmsg->type == MESSAGE_IFRAME_REQUEST) {
                    mprintf("Request for i-frame found: Creating iframe\n");
                    if( fmsg->reinitialize_encoder )
                    {
                        update_encoder = true;
                    } else
                    {
                        wants_iframe = true;
                    }
                } else if (fmsg->type == CMESSAGE_QUIT) {
                    // Client requested to exit, it's time to disconnect
                    mprintf("Client Quit\n");
                    connected = false;
                }
            }
        }

        mprintf("Disconnected\n");

        DestroyInputDevice(input_device);

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
