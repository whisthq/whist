#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <windows.h>

#include "../include/fractal.h"
#include "../include/videocapture.h"
#include "../include/wasapicapture.h"
#include "../include/videoencode.h"
#include "../include/audioencode.h"
#include "../include/dxgicapture.h"

#pragma comment (lib, "ws2_32.lib")

#define USE_GPU 0
#define USE_MONITOR 0
#define ENCODE_TYPE NVENC_ENCODE

volatile static bool connected;
volatile static double max_mbps;
volatile static int gop_size = 10;

char buf[LARGEST_FRAME_SIZE];

#define MAX_VIDEO_INDEX 500
#define VIDEO_BUFFER_SIZE 25
struct RTPPacket video_buffer[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];
int video_buffer_packet_len[VIDEO_BUFFER_SIZE][MAX_VIDEO_INDEX];

#define MAX_AUDIO_INDEX 10
#define AUDIO_BUFFER_SIZE 1000

struct RTPPacket audio_buffer[AUDIO_BUFFER_SIZE][MAX_AUDIO_INDEX];
int audio_buffer_packet_len[AUDIO_BUFFER_SIZE][MAX_AUDIO_INDEX];

SDL_mutex* packet_mutex;

static int ReplayPacket(struct SocketContext* context, struct RTPPacket* packet, int len) {
    if (len > sizeof(struct RTPPacket)) {
        mprintf("Len too long!\n");
        return -1;
    }
    SDL_LockMutex(packet_mutex);
    int sent_size = sendto(context->s, packet, len, 0, (struct sockaddr*)(&context->addr), sizeof(context->addr));
    SDL_UnlockMutex(packet_mutex);

    if (sent_size < 0) {
        mprintf("Could not replay packet!\n");
        return -1;
    }
}

static int SendPacket(struct SocketContext* context, FractalPacketType type, uint8_t* data, int len, int id, double time) {
    if (id <= 0) {
        mprintf("IDs must be positive!\n");
        return -1;
    }

    int payload_size;
    int curr_index = 0, i = 0;

    clock packet_timer;
    StartTimer(&packet_timer);

    while (curr_index < len) {
        struct RTPPacket l_packet = { 0 };
        int l_len = 0;

        int* packet_len = &l_len;
        struct RTPPacket* packet = &l_packet;
        if (type == PACKET_AUDIO) {
            if (i >= MAX_AUDIO_INDEX) {
                mprintf("Audio index too long!\n");
                return -1;
            }
            else {
                packet = &audio_buffer[id % AUDIO_BUFFER_SIZE][i];
                packet_len = &audio_buffer_packet_len[id % AUDIO_BUFFER_SIZE][i];
            }
        }
        else if (type == PACKET_VIDEO) {
            if (i >= MAX_VIDEO_INDEX) {
                mprintf("Video index too long!\n");
                return -1;
            }
            else {
                packet = &video_buffer[id % VIDEO_BUFFER_SIZE][i];
                packet_len = &video_buffer_packet_len[id % VIDEO_BUFFER_SIZE][i];
            }
        }
        payload_size = min(MAX_PAYLOAD_SIZE, (len - curr_index));

        memcpy(packet->data, data + curr_index, payload_size);
        packet->type = type;
        packet->index = i;
        packet->payload_size = payload_size;
        packet->id = id;
        packet->is_ending = curr_index + payload_size == len;
        int packet_size = sizeof(*packet) - sizeof(packet->data) + packet->payload_size;
        packet->hash = Hash((char*)packet + sizeof(packet->hash), packet_size - sizeof(packet->hash));

        SDL_LockMutex(packet_mutex);
        *packet_len = packet_size;
        int sent_size = sendto(context->s, packet, packet_size, 0, (struct sockaddr*)(&context->addr), sizeof(context->addr));
        SDL_UnlockMutex(packet_mutex);

        if (sent_size < 0) {
            int error = WSAGetLastError();
            mprintf("Unexpected Packet Error: %d\n", error);
            return -1;
        }

        i++;
        curr_index += payload_size;
        while (time > 0.0 && GetTimer(packet_timer) / time < 1.0 * curr_index / len);
    }

    return 0;
}


static int32_t SendVideo(void* opaque) {
    struct SocketContext socketContext = *(struct SocketContext*) opaque;
    int slen = sizeof(socketContext.addr), id = 1;

    // Init DXGI Device
    DXGIDevice* device = (DXGIDevice*)malloc(sizeof(DXGIDevice));
    memset(device, 0, sizeof(DXGIDevice));
    if (CreateDXGIDevice(device) < 0) {
        mprintf("Error Creating DXGI Device\n");
        return -1;
    }

    // Init FFMPEG Encoder
    int current_bitrate = STARTING_BITRATE;
    encoder_t* encoder;
    encoder = create_video_encoder(device->width, device->height,
        device->width, device->height, device->width * current_bitrate, gop_size, ENCODE_TYPE);

    bool update_encoder = false;

    double worst_fps = 40.0;
    int ideal_bitrate = current_bitrate;
    int bitrate_tested_frames = 0;
    int bytes_tested_frames = 0;

    clock previous_frame_time;
    int previous_frame_size = 0;

    int consecutive_capture_screen_errors = 0;

    while (connected) {
        HRESULT hr = CaptureScreen(device);
        clock server_frame_timer;
        StartTimer(&server_frame_timer);

        if (hr == S_OK) {
            consecutive_capture_screen_errors = 0;

            if (update_encoder) {
                destroy_video_encoder(encoder);
                encoder = create_video_encoder(device->width, device->height, device->width, device->height, device->width * current_bitrate, gop_size, ENCODE_TYPE);
                update_encoder = false;
            }

            clock t;
            StartTimer(&t);
            video_encoder_encode(encoder, device->frame_data.pBits);
            //mprintf("Encode Time: %f\n", GetTimer(t));

            bitrate_tested_frames++;
            bytes_tested_frames += encoder->packet.size;

            if (encoder->packet.size != 0) {
                double delay = -1.0;

                if (previous_frame_size > 0) {
                    double frame_time = GetTimer(previous_frame_time);
                    StartTimer(&previous_frame_time);
                    double mbps = previous_frame_size * 8.0 / 1024.0 / 1024.0 / frame_time;
                    // previousFrameSize * 8.0 / 1024.0 / 1024.0 / IdealTime = max_mbps
                    // previousFrameSize * 8.0 / 1024.0 / 1024.0 / max_mbps = IdealTime
                    double transmit_time = previous_frame_size * 8.0 / 1024.0 / 1024.0 / max_mbps;

                    double average_frame_size = 1.0 * bytes_tested_frames / bitrate_tested_frames;
                    double current_trasmit_time = previous_frame_size * 8.0 / 1024.0 / 1024.0 / max_mbps;
                    double current_fps = 1.0 / current_trasmit_time;

                    delay = transmit_time - frame_time;
                    delay = min(delay, 0.004);

                    //mprintf("Size: %d, MBPS: %f, VS MAX MBPS: %f, Time: %f, Transmit Time: %f, Delay: %f\n", previous_frame_size, mbps, max_mbps, frame_time, transmit_time, delay);

                    if ((current_fps < worst_fps || ideal_bitrate > current_bitrate) && bitrate_tested_frames > 20) {
                        // Rather than having lower than the worst acceptable fps, find the ratio for what the bitrate should be
                        double ratio_bitrate = current_fps / worst_fps;
                        int new_bitrate = (int)(ratio_bitrate * current_bitrate);
                        if (abs(new_bitrate - current_bitrate) / new_bitrate > 0.05) {
                            mprintf("Updating bitrate from %d to %d\n", current_bitrate, new_bitrate);
                            //current_bitrate = new_bitrate;
                            //update_encoder = true;

                            bitrate_tested_frames = 0;
                            bytes_tested_frames = 0;
                        }
                    }
                }

                int frame_size = sizeof(Frame) + encoder->packet.size;
                if (frame_size > LARGEST_FRAME_SIZE) {
                    mprintf("Frame too large: %d\n", frame_size);
                }
                else {
                    Frame* frame = buf;
                    frame->width = device->width;
                    frame->height = device->height;
                    frame->size = encoder->packet.size;
                    memcpy(frame->compressed_frame, encoder->packet.data, encoder->packet.size);
                    
                    if (SendPacket(&socketContext, PACKET_VIDEO, frame, frame_size, id, delay) < 0) {
                        mprintf("Could not send video frame\n");
                    }
                    else {
                        //mprintf("Sent size %d\n", encoder->packet.size);
                        previous_frame_size = encoder->packet.size;
                    }
                    float server_frame_time = GetTimer(server_frame_timer);
                    //mprintf("Server Frame Time for ID %d: %f\n", id, server_frame_time);
                }
            }

            id++;
            ReleaseScreen(device);
        }
        else if (hr != DXGI_ERROR_WAIT_TIMEOUT) {
            mprintf("ERROR: %d\n", WSAGetLastError());
            consecutive_capture_screen_errors++;
            if (consecutive_capture_screen_errors == 3) {
                mprintf("DXGI errored too many times!\n");
                break;
            }
        }
    }

    DestroyDXGIDevice(device);
    return 0;
}

static int32_t SendAudio(void* opaque) {
    struct SocketContext context = *(struct SocketContext*) opaque;
    int slen = sizeof(context.addr), id = 1;

    wasapi_device* audio_device = (wasapi_device*)malloc(sizeof(struct wasapi_device));
    audio_device = CreateAudioDevice(audio_device);
    StartAudioDevice(audio_device);

    HRESULT hr = CoInitialize(NULL);
    DWORD dwWaitResult;
    UINT32 nNumFramesToRead, nNextPacketSize, nBlockAlign = audio_device->pwfx->nBlockAlign;
    DWORD dwFlags;

    while (connected) {
        for (hr = audio_device->pAudioCaptureClient->lpVtbl->GetNextPacketSize(audio_device->pAudioCaptureClient, &nNextPacketSize);
            SUCCEEDED(hr) && nNextPacketSize > 0;
            hr = audio_device->pAudioCaptureClient->lpVtbl->GetNextPacketSize(audio_device->pAudioCaptureClient, &nNextPacketSize)
            ) {
            audio_device->pAudioCaptureClient->lpVtbl->GetBuffer(
                audio_device->pAudioCaptureClient,
                &audio_device->pData, &nNumFramesToRead,
                &dwFlags, NULL, NULL);

            audio_device->audioBufSize = nNumFramesToRead * nBlockAlign;

            if (audio_device->audioBufSize != 0) {
                if (SendPacket(&context, PACKET_AUDIO, audio_device->pData, audio_device->audioBufSize, id, -1.0) < 0) {
                    mprintf("Could not send audio frame\n");
                }
            }

            audio_device->pAudioCaptureClient->lpVtbl->ReleaseBuffer(
                audio_device->pAudioCaptureClient,
                nNumFramesToRead);

            id++;
        }
        dwWaitResult = WaitForSingleObject(audio_device->hWakeUp, INFINITE);
    }
    DestroyAudioDevice(audio_device);
    return 0;
}


int main(int argc, char* argv[])
{
    initMultiThreadedPrintf();

    while (true) {
        // initialize the windows socket library if this is a windows client
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            mprintf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
            return -1;
        }

        struct SocketContext PacketReceiveContext = { 0 };
        if (CreateUDPContext(&PacketReceiveContext, "S", "", 1, -1) < 0) {
            exit(1);
        }

        struct SocketContext PacketSendContext = { 0 };
        if (CreateUDPContext(&PacketSendContext, "S", "", 5, -1) < 0) {
            exit(1);
        }

        connected = true;
        max_mbps = START_MAX_MBPS;

        packet_mutex = SDL_CreateMutex();

        SDL_Thread* send_video = SDL_CreateThread(SendVideo, "SendVideo", &PacketSendContext);
        SDL_Thread* send_audio = SDL_CreateThread(SendAudio, "SendAudio", &PacketSendContext);

        struct FractalClientMessage fmsgs[6];
        struct FractalClientMessage fmsg;
        int slen = sizeof(PacketReceiveContext.addr), i = 0, j = 0, active = 0;
        FractalStatus status;

        clock last_ping;
        StartTimer(&last_ping);

        clock ack_timer;
        SendAck(&PacketReceiveContext, 1);
        StartTimer(&ack_timer);

        while (connected) {
            if (GetTimer(ack_timer) * 1000.0 > ACK_REFRESH_MS) {
                SendAck(&PacketReceiveContext, 1);
                StartTimer(&ack_timer);
            }

            if (GetTimer(last_ping) > 1.5) {
                mprintf("Client connection dropped.\n");
                connected = false;
                break;
            }

            memset(&fmsg, 0, sizeof(fmsg));
            if (recvfrom(PacketReceiveContext.s, &fmsg, sizeof(fmsg), 0, (struct sockaddr*)(&PacketReceiveContext.addr), &slen) > 0) {
                if (fmsg.type == MESSAGE_KEYBOARD) {
                    if (active) {
                        fmsgs[j] = fmsg;
                        if (fmsg.keyboard.pressed) {
                            if (fmsg.keyboard.code != fmsgs[j - 1].keyboard.code) {
                                j++;
                            }
                        }
                        else {
                            status = ReplayUserInput(fmsgs, j + 1);
                            active = 0;
                            j = 0;
                        }
                    }
                    else {
                        fmsgs[0] = fmsg;
                        if (fmsg.keyboard.pressed && (fmsg.keyboard.code >= 224 && fmsg.keyboard.code <= 231)) {
                            active = 1;
                            j++;
                        }
                        else {
                            status = ReplayUserInput(fmsgs, 1);
                        }
                    }
                }
                else if (fmsg.type == MESSAGE_MBPS) {
                    max_mbps = fmsg.mbps;
                }
                else if (fmsg.type == MESSAGE_PING) {
                    mprintf("Ping Received - ID %d\n", fmsg.ping_id);

                    FractalServerMessage fmsg_response = { 0 };
                    fmsg_response.type = MESSAGE_PONG;
                    fmsg_response.ping_id = fmsg.ping_id;
                    StartTimer(&last_ping);

                    if (SendPacket(&PacketSendContext, PACKET_MESSAGE, &fmsg_response, sizeof(fmsg_response), 1, -1) < 0) {
                        mprintf("Could not send Pong\n");
                    }
                }
                else if (fmsg.type == MESSAGE_DIMENSIONS) {
                    int width = fmsg.dimensions.width;
                    int height = fmsg.dimensions.height;
                    mprintf("Changing dimensions: %d by %d\n", width, height);
                }
                else if (fmsg.type == MESSAGE_QUIT) {
                    mprintf("Client Quit\n");
                    connected = false;
                }
                else if (fmsg.type == MESSAGE_AUDIO_NACK) {
                    mprintf("Audio NACK requested for: ID %d Index %d\n", fmsg.nack_data.id, fmsg.nack_data.index);
                    struct RTPPacket *audio_packet = &audio_buffer[fmsg.nack_data.id % AUDIO_BUFFER_SIZE][fmsg.nack_data.index];
                    int len = audio_buffer_packet_len[fmsg.nack_data.id % AUDIO_BUFFER_SIZE][fmsg.nack_data.index];
                    if (audio_packet->id == fmsg.nack_data.id) {
                        mprintf("NACKed packet found of length %d. Relaying!\n", len);
                        ReplayPacket(&PacketSendContext, audio_packet, len);
                    }
                    else {
                        mprintf("NACKed packet not found, ID %d was located instead.\n", audio_packet->id);
                    }
                }
                else if (fmsg.type == MESSAGE_VIDEO_NACK) {
                    mprintf("Video NACK requested for: ID %d Index %d\n", fmsg.nack_data.id, fmsg.nack_data.index);
                    struct RTPPacket* video_packet = &video_buffer[fmsg.nack_data.id % VIDEO_BUFFER_SIZE][fmsg.nack_data.index];
                    int len = video_buffer_packet_len[fmsg.nack_data.id % VIDEO_BUFFER_SIZE][fmsg.nack_data.index];
                    if (video_packet->id == fmsg.nack_data.id) {
                        mprintf("NACKed packet found of length %d. Relaying!\n", len);
                        ReplayPacket(&PacketSendContext, video_packet, len);
                    }
                    else {
                        mprintf("NACKed packet not found, ID %d was located instead.\n", video_packet->id);
                    }
                }
                else {
                    fmsgs[0] = fmsg;
                    status = ReplayUserInput(fmsgs, 1);
                }
            }
        }

        SDL_WaitThread(send_video, NULL);
        SDL_WaitThread(send_audio, NULL);

        SDL_DestroyMutex(packet_mutex);

        closesocket(PacketReceiveContext.s);
        closesocket(PacketSendContext.s);

        WSACleanup();
    }

    destroyMultiThreadedPrintf();

    return 0;
}