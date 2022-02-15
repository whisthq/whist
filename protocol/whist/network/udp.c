#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS  // unportable Windows warnings, needs to
                                         // be at the very top
#endif

/*
============================
Includes
============================
*/

#include "udp.h"
#include <whist/utils/aes.h>
#include <whist/utils/fec.h>
#include <whist/network/network_algorithm.h>
#include <whist/network/ringbuffer.h>
#include <whist/logging/log_statistic.h>
#include <whist/network/throttle.h>
#include <stddef.h>

#ifndef _WIN32
#include <fcntl.h>
#endif

/*
============================
Defines
============================
*/

typedef enum {
    UDP_WHIST_SEGMENT,
    UDP_NACK,
    UDP_BITARRAY_NACK,
    UDP_STREAM_RESET,
    UDP_PING,
    UDP_PONG,
    UDP_NETWORK_SETTINGS,
    UDP_CONNECTION_ATTEMPT,
    UDP_CONNECTION_CONFIRMATION,
} UDPPacketType;

// A struct for UDPPacket,
// which is what the UDPContext will communicate with
// If this is changed, get_udp_packet_size may need to be updated
typedef struct {
    // Metadata about the packet segment
    UDPPacketType type;

    // The data itself
    union {
        // UDP_WHIST_SEGMENT
        // A segment of a WhistPacket
        WhistSegment udp_whist_segment_data;

        // UDP_NACK
        struct {
            WhistPacketType whist_type;
            int id;
            unsigned short index;
        } udp_nack_data;

        // UDP_BITARRAY_NACK
        struct {
            WhistPacketType type;
            int id;
            int index;
            int numBits;
            unsigned char ba_raw[BITS_TO_CHARS(max(MAX_VIDEO_PACKETS, MAX_AUDIO_PACKETS))];
        } udp_bitarray_nack_data;

        // UDP_STREAM_RESET
        struct {
            WhistPacketType whist_type;
            // The biggest ID that failed to be received
            int greatest_failed_id;
        } udp_stream_reset_data;

        // UDP_PING
        struct {
            int id;
            timestamp_us original_timestamp;
        } udp_ping_data;

        // UDP_PONG
        struct {
            int id;
        } udp_pong_data;

        // UDP_NETWORK_SETTINGS
        struct {
            NetworkSettings network_settings;
        } udp_network_settings_data;
    };
} UDPPacket;

// The struct that actually gets sent over the network
typedef struct {
    // AES Metadata needed to decrypt the payload
    AESMetadata aes_metadata;
    // Size of the payload
    int payload_size;
    // The data getting transmitted within the UDPNetworkPacket,
    // Which will be an encrypted UDPPacket
    char payload[sizeof(UDPPacket) + MAX_ENCRYPTION_SIZE_INCREASE];
} UDPNetworkPacket;

// Size of the UDPPacket header, excluding the payload
#define UDPNETWORKPACKET_HEADER_SIZE ((int)(offsetof(UDPNetworkPacket, payload)))

typedef struct {
    bool pending_stream_reset;
    int greatest_failed_id;
} StreamResetData;

// An instance of the UDP Context
typedef struct {
    int timeout;
    SOCKET socket;
    int ack;
    WhistMutex mutex;
    char binary_aes_private_key[16];
    NetworkThrottleContext* network_throttler;

    double fec_packet_ratios[NUM_PACKET_TYPES];

    // last_addr is the last address we received a packet from.
    // When connected == true, this must equal connection_addr
    bool connected;
    struct sockaddr_in last_addr;
    struct sockaddr_in connection_addr;

    // Ping/Pong data and timers
    int consecutive_ping_failures;
    int last_ping_id;
    int last_pong_id;
    WhistTimer last_ping_timer;
    WhistTimer last_new_ping_timer;
    WhistTimer last_received_ping_timer;
    bool connection_lost;

    // Latency Calculation (Only used on server)
    WhistMutex timestamp_mutex;
    timestamp_us last_ping_client_time;
    timestamp_us last_ping_server_time;

    int last_start_of_stream_id[NUM_PACKET_TYPES];

    // Nack Buffer Data
    UDPPacket** nack_buffers[NUM_PACKET_TYPES];
    // Whether or not a nack buffer is being used right now
    bool** nack_buffer_valid[NUM_PACKET_TYPES];
    // This mutex will protect the data in nack_buffers
    WhistMutex nack_mutex[NUM_PACKET_TYPES];
    int nack_num_buffers[NUM_PACKET_TYPES];
    int nack_buffer_max_indices[NUM_PACKET_TYPES];
    int nack_buffer_max_payload_size[NUM_PACKET_TYPES];
    RingBuffer* ring_buffers[NUM_PACKET_TYPES];

    StreamResetData reset_data[NUM_PACKET_TYPES];

    // Holds packets that pending to be received
    WhistPacket pending_packets[NUM_PACKET_TYPES];
    // Whether or not pending_packets[i] contains a pending packet
    bool has_pending_packet[NUM_PACKET_TYPES];

    // The current network settings the other side asked us to follow
    NetworkSettings current_network_settings;
    // The network settings we're asking the other side to follow
    NetworkSettings desired_network_settings;
    WhistTimer last_network_settings_time;
} UDPContext;

// Define how many times to retry sending a UDP packet in case of Error 55 (buffer full). The
// current value (5) is an arbitrary choice that was found to work well in practice.
#define RETRIES_ON_BUFFER_FULL 5

// Define the bitrate specified to be maintained for each and every 0.5 ms internal.
#define UDP_NETWORK_THROTTLER_BUCKET_MS 0.5
// The time in seconds between network statistics requests
#define STATISTICS_SECONDS 5.0
// TODO: Get this out of udp.c somehow
// NOTE that this is matching ./client/audio.c
#define MAX_NUM_AUDIO_FRAMES 10

// The amount to weigh a older pings' latency,
// on the ewma latency value
#define PING_LAMBDA 0.6

// How often should the client send connection attempts
#define CONNECTION_ATTEMPT_INTERVAL_MS 5
// How many confirmation packets the server should respond with,
// When it receives a valid client connection attempt
#define NUM_CONFIRMATION_MESSAGES 10

/*
============================
Globals
============================
*/

// TODO: Remove bad globals
extern unsigned short port_mappings[USHRT_MAX + 1];
extern volatile bool connected;
volatile double latency;

/*
============================
Private Functions
============================
*/

/**
 * @brief                          Create a UDP Server Context,
 *                                 which will wait connection_timeout_ms
 *                                 for a client to connect.
 *
 * @param context                  The UDP context to connect with
 * @param port                     The port to open up
 * @param connection_timeout_ms    The amount of time to wait for a client
 *
 * @returns                        -1 on failure, 0 on success
 *
 * @note                           This may overwrite the timeout on context->socket,
 *                                 Use set_timeout to restore it
 */
int create_udp_server_context(UDPContext* context, int port, int connection_timeout_ms);

/**
 * @brief                          Create a UDP Client Context,
 *                                 which will try to connect for connection_timeout_ms
 *
 * @param context                  The UDP context to connect with
 * @param destination              The IP address to connect to
 * @param port                     The port to connect to
 * @param connection_timeout_ms    The amount of time to try to connect
 *
 * @returns                        -1 on failure, 0 on success
 *
 * @note                           This may overwrite the timeout on context->socket,
 *                                 Use set_timeout to restore it
 */
int create_udp_client_context(UDPContext* context, char* destination, int port,
                              int connection_timeout_ms);

/**
 * @brief                        Encrypts and sends a UDPPacket over the network
 *
 * @param udp_packet             The UDPPacket to send.
 *                               This buffer is expected to be of size sizeof(UDPPacket)
 */
static int udp_send_udp_packet(UDPContext* context, UDPPacket* udp_packet);

/**
 * @brief                        Gets and decrypts a UDPPacket over the network
 *
 * @param udp_packet             The UDPPacket buffer to write to.
 *                               This buffer is expected to be of size sizeof(UDPPacket)
 *
 * @returns                      True if a packet was received and written to,
 *                               False if no packet was received
 *
 * @note                         This will call recv on context->socket, which will
 *                               wait for as long as the socket's most recent set_timeout
 */
static bool udp_get_udp_packet(UDPContext* context, UDPPacket* udp_packet);

/**
 * @brief                        Returns the size, in bytes, of the relevant part of
 *                               the UDPPacket, that must be sent over the network
 *
 * @param udp_packet             The udp packet whose size we're interested in
 *
 * @returns                      The size N of the udp packet. Only the first N
 *                               bytes needs to be recovered on the receiving side,
 *                               in order to read the contents of the udp packet
 */
static int get_udp_packet_size(UDPPacket* udp_packet);

// TODO: document
static void udp_update_ping(UDPContext* context);

/*
============================
UDP Message Handling
============================
*/

/**
 * @brief                   Handle any UDPPacket that's not a Whist segment,
 *                          e.g. nack messages and network setting updates
 *
 * @param context           The UDPContext to handle the message
 * @param packet            The UDPPacket to handle
 */
static void udp_handle_message(UDPContext* context, UDPPacket* packet);

// Handler functions for the various UDP messages
static void udp_handle_nack(UDPContext* context, WhistPacketType type, int id, int index);
static void udp_handle_ping(UDPContext* context, int id, timestamp_us timestamp);
static void udp_handle_pong(UDPContext* context, int id);
static void udp_handle_stream_reset(UDPContext* context, WhistPacketType type,
                                    int greatest_failed_id);

/*
============================
RingBuffer Lambda Functions
============================
*/

/**
 * @brief                    Send a nack to the server indicating that the client
 *                           is missing the packet with given type, ID, and index
 *
 * @param socket_context     the context we're sending the nack over
 * @param type               type of packet
 * @param ID                 ID of packet
 * @param index              index of packet
 */
static void udp_nack_packet(SocketContext* socket_context, WhistPacketType type, int id,
                            int index) {
    UDPContext* context = (UDPContext*)socket_context->context;
    // Nack for the given packet by sending a UDPPacket of type UDP_NACK
    UDPPacket packet = {0};
    packet.type = UDP_NACK;
    packet.udp_nack_data.whist_type = type;
    packet.udp_nack_data.id = id;
    packet.udp_nack_data.index = index;
    udp_send_udp_packet(context, &packet);
}

static void udp_request_stream_reset(SocketContext* socket_context, WhistPacketType type,
                                     int greatest_failed_id) {
    UDPContext* context = (UDPContext*)socket_context->context;
    // Tell the server the client fell too far behind
    UDPPacket packet = {0};
    packet.type = UDP_STREAM_RESET;
    packet.udp_stream_reset_data.whist_type = type;
    packet.udp_stream_reset_data.greatest_failed_id = greatest_failed_id;
    udp_send_udp_packet(context, &packet);
}

/*
============================
UDP Implementation of Network.h Interface
============================
*/

static bool udp_update(void* raw_context) {
    /*
     * Read a WhistPacket from the socket, decrypt it if necessary, and store the decrypted data for
     * the next get_packet call.
     * TODO: specify that we are reading WhistUDPPackets from the UDP socket and handle combining
     * the WhistUDPPackets into WhistPackets. Return NULL if no full packet is available, and the
     * full WhistPacket otherwise.
     * TODO: handle nacking by creating ring buffers for packet types that require nacking (audio
     * and video)
     */
    FATAL_ASSERT(raw_context != NULL);
    UDPContext* context = (UDPContext*)raw_context;

    if (context->connection_lost) {
        return false;
    }

    // *************
    // Keep the connection alive and check connection health with pings
    // *************

    // Check if we need to ping
    // This implicitly keeps the connection alive as well
    udp_update_ping(context);

    // udp_update_ping may have detected a newly lost connection
    if (context->connection_lost) {
        return false;
    }

    // *************
    // Potentially ask for new network settings based on the current network statistics
    // *************

    // TODO: Make this not PACKET_VIDEO specific
    // TODO: Probably move statistics calculations into udp.c as well,
    //       but it depends on how much statistics are per-frame-specific
    if (context->ring_buffers[PACKET_VIDEO] != NULL) {
        // Initialize desired_network_settings if it is not done yet.
        if (context->desired_network_settings.bitrate == 0) {
            context->desired_network_settings = get_starting_network_settings();
        }
        if (get_timer(&context->last_network_settings_time) > STATISTICS_SECONDS) {
            // Get network statistics and desired network settings
            NetworkStatistics network_statistics =
                get_network_statistics(context->ring_buffers[PACKET_VIDEO]);
            NetworkSettings desired_network_settings =
                get_desired_network_settings(network_statistics);
            // If we have new desired network settings for the other socket,
            if (memcmp(&context->desired_network_settings, &desired_network_settings,
                       sizeof(NetworkSettings)) != 0) {
                context->desired_network_settings = desired_network_settings;
                // Ask for the other socket to match our desires
                UDPPacket network_settings_packet;
                network_settings_packet.type = UDP_NETWORK_SETTINGS;
                network_settings_packet.udp_network_settings_data.network_settings =
                    desired_network_settings;
                udp_send_udp_packet(context, &network_settings_packet);
            }
            start_timer(&context->last_network_settings_time);
        }
    }

    // *************
    // Pull a packet from the network, if any is there
    // *************

    UDPPacket udp_packet;
    bool received_packet = udp_get_udp_packet(context, &udp_packet);

    if (received_packet) {
        // if the packet is a whist_segment, store the data to give later via get_packet
        // Otherwise, pass it to udp_handle_message
        if (udp_packet.type == UDP_WHIST_SEGMENT) {
            WhistPacketType packet_type = udp_packet.udp_whist_segment_data.whist_type;

            // If there's a ringbuffer, store in the ringbuffer to reconstruct the original packet
            if (context->ring_buffers[packet_type] != NULL) {
                ring_buffer_receive_segment(context->ring_buffers[packet_type],
                                            &udp_packet.udp_whist_segment_data);
            } else {
                FATAL_ASSERT(udp_packet.udp_whist_segment_data.num_indices == 1);
                FATAL_ASSERT(udp_packet.udp_whist_segment_data.num_fec_indices == 0);
                // if there is no ring buffer (packet is message), store it in the 1-packet buffer
                // instead memcpy the segment_data (WhistPacket*) into pending_packets
                memcpy(&context->pending_packets[packet_type],
                       &udp_packet.udp_whist_segment_data.segment_data,
                       udp_packet.udp_whist_segment_data.segment_size);
                if (context->has_pending_packet[packet_type]) {
                    LOG_ERROR(
                        "get_packet has not been called, unclaimed PACKET_MESSAGE being "
                        "overwritten!");
                } else {
                    context->has_pending_packet[packet_type] = true;
                }
            }
        } else {
            // Handle the UDP message
            udp_handle_message(context, &udp_packet);
        }
    }

    // *************
    // Try nacking or requesting a stream reset
    // *************

    for (int i = 0; i < NUM_PACKET_TYPES; i++) {
        if (context->ring_buffers[i] != NULL) {
            // At the moment we only nack for video
            // TODO: Make this not packet-type-dependent
            if (i == (int)PACKET_VIDEO) {
                try_recovering_missing_packets_or_frames(context->ring_buffers[i], latency,
                                                         &context->desired_network_settings);
            }
        }
    }

    return true;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
static int udp_send_packet(void* raw_context, WhistPacketType packet_type,
                           void* whist_packet_payload, int whist_packet_payload_size, int packet_id,
                           bool start_of_stream) {
    FATAL_ASSERT(raw_context != NULL);
    UDPContext* context = (UDPContext*)raw_context;
    FATAL_ASSERT(context != NULL);

    if (context->connection_lost) {
        return -1;
    }

    // Update what the most recent start of stream ID was
    if (start_of_stream) {
        context->last_start_of_stream_id[packet_type] = packet_id;
    }

    // The caller should either start counting at 1,
    // Or use -1 for a packet without an ID
    FATAL_ASSERT(packet_id != 0);

    // Get the nack_buffer, if there is one for this type of packet
    UDPPacket* nack_buffer = NULL;

    int type_index = (int)packet_type;
    FATAL_ASSERT(type_index < NUM_PACKET_TYPES);
    if (context->nack_buffers[type_index] != NULL) {
        // Sending payloads that must be split into multiple packets,
        // is only allowed for WhistPacketType's that have a nack buffer
        // This includes allowing the application to fec_ratio at all
        nack_buffer =
            context->nack_buffers[type_index][packet_id % context->nack_num_buffers[type_index]];
        // Packets that are using a nack buffer need a positive ID
        FATAL_ASSERT(packet_id > 0);
    }

    // Construct the WhistPacket based on the parameters given
    WhistPacket* whist_packet = allocate_region(PACKET_HEADER_SIZE + whist_packet_payload_size);
    whist_packet->id = packet_id;
    whist_packet->type = packet_type;
    whist_packet->payload_size = whist_packet_payload_size;
    memcpy(whist_packet->data, whist_packet_payload, whist_packet_payload_size);
    int whist_packet_size = get_packet_size(whist_packet);

    // Calculate number of packets needed to send the payload, rounding up.
    int num_indices = whist_packet_size == 0
                          ? 1
                          : (int)(whist_packet_size / MAX_PACKET_SEGMENT_SIZE +
                                  (whist_packet_size % MAX_PACKET_SEGMENT_SIZE == 0 ? 0 : 1));

    // Calculate the number of FEC packets we'll be using, if any
    // A nack buffer is required to use FEC
    int num_fec_packets = 0;
    double fec_packet_ratio = context->fec_packet_ratios[packet_type];
    if (nack_buffer && fec_packet_ratio > 0.0) {
        num_fec_packets = get_num_fec_packets(num_indices, fec_packet_ratio);
    }

    int num_total_packets = num_indices + num_fec_packets;

// Feel free to increase this #define if the fatal assert gets triggered
#define MAX_TOTAL_PACKETS 4096
    char* buffers[MAX_TOTAL_PACKETS];
    int buffer_sizes[MAX_TOTAL_PACKETS];
    FATAL_ASSERT(num_total_packets < MAX_TOTAL_PACKETS);

    // If nack buffer can't hold a packet with that many indices,
    // OR the original buffer is illegally large
    // OR there's no nack buffer but it's a packet that needed to be split up,
    // THEN there's a problem and we LOG_ERROR
    if ((nack_buffer && num_total_packets > context->nack_buffer_max_indices[type_index]) ||
        (nack_buffer && whist_packet_size > context->nack_buffer_max_payload_size[type_index]) ||
        (!nack_buffer && num_total_packets > 1)) {
        LOG_ERROR("Packet is too large to send the payload! %d/%d", num_indices, num_total_packets);
        deallocate_region(whist_packet);
        return -1;
    }

    FECEncoder* fec_encoder = NULL;
    if (num_fec_packets > 0) {
        fec_encoder = create_fec_encoder(num_indices, num_fec_packets, MAX_PACKET_SEGMENT_SIZE);
        // Pass the buffer that we'll be encoding with FEC
        fec_encoder_register_buffer(fec_encoder, (char*)whist_packet, whist_packet_size);
        // If using FEC, populate the UDP payload buffers with the FEC encoded buffers
        fec_get_encoded_buffers(fec_encoder, (void**)buffers, buffer_sizes);
    } else {
        // When not using FEC, split up the packets using MAX_PACKET_SEGMENT_SIZE
        int current_position = 0;
        for (int packet_index = 0; packet_index < num_indices; packet_index++) {
            // Populate the buffers directly when not using FEC
            int udp_packet_payload_size =
                min(whist_packet_size - current_position, MAX_PACKET_SEGMENT_SIZE);
            buffers[packet_index] = (char*)whist_packet + current_position;
            buffer_sizes[packet_index] = udp_packet_payload_size;
            // Progress the pointer by this payload's size
            current_position += udp_packet_payload_size;
        }
        FATAL_ASSERT(current_position == whist_packet_size);
    }

    // Send all the packets, and write them into the nack buffer if there is one
    for (int packet_index = 0; packet_index < num_total_packets; packet_index++) {
        if (nack_buffer) {
            // Lock on a per-loop basis to not starve nack() calls
            whist_lock_mutex(context->nack_mutex[type_index]);
        }

        // The UDPPacket that we will construct
        UDPPacket local_packet;
        UDPPacket* packet = &local_packet;

        // Potentially use the nack buffer instead though
        if (nack_buffer) {
            packet = &nack_buffer[packet_index];
            context
                ->nack_buffer_valid[type_index][packet_id % context->nack_num_buffers[type_index]]
                                   [packet_index] = true;
        }

        // Construct the UDPPacket, potentially into the nack buffer
        packet->type = UDP_WHIST_SEGMENT;
        packet->udp_whist_segment_data.whist_type = packet_type;
        packet->udp_whist_segment_data.id = packet_id;
        packet->udp_whist_segment_data.index = (unsigned short)packet_index;
        packet->udp_whist_segment_data.num_indices = (unsigned short)num_total_packets;
        packet->udp_whist_segment_data.num_fec_indices = (unsigned short)num_fec_packets;
        packet->udp_whist_segment_data.is_a_nack = false;
        packet->udp_whist_segment_data.segment_size = buffer_sizes[packet_index];

        FATAL_ASSERT(packet->udp_whist_segment_data.segment_size <=
                     sizeof(packet->udp_whist_segment_data.segment_data));
        memcpy(packet->udp_whist_segment_data.segment_data, buffers[packet_index],
               buffer_sizes[packet_index]);

        // Send the packet
        // We don't need to propagate the return code because it's lossy anyway,
        // The client will just have to nack
        udp_send_udp_packet(context, packet);

        if (nack_buffer) {
            whist_unlock_mutex(context->nack_mutex[type_index]);
        }
    }

    // Cleanup
    if (fec_encoder) {
        destroy_fec_encoder(fec_encoder);
    }
    deallocate_region(whist_packet);

    return 0;
}

static void* udp_get_packet(void* raw_context, WhistPacketType type) {
    FATAL_ASSERT(raw_context != NULL);
    UDPContext* context = (UDPContext*)raw_context;
    RingBuffer* ring_buffer = context->ring_buffers[type];

    if (context->connection_lost) {
        return NULL;
    }

    // TODO: This function is a bit too audio/video specific,
    // Rather than data-agnostic

    switch (type) {
        case PACKET_MESSAGE: {
            // If there's a pending whist packet,
            if (context->has_pending_packet[PACKET_MESSAGE]) {
                // Give them a copy, consuming the pending packet in the process
                WhistPacket* pending_whist_packet = &context->pending_packets[PACKET_MESSAGE];
                WhistPacket* whist_packet =
                    (WhistPacket*)malloc(get_packet_size(pending_whist_packet));
                memcpy(whist_packet, pending_whist_packet, get_packet_size(pending_whist_packet));
                context->has_pending_packet[PACKET_MESSAGE] = false;
                return whist_packet;
            } else {
                // Otherwise, return NULL
                return NULL;
            }
            break;
        }
        case PACKET_AUDIO: {
            // First, catch up audio if we're behind
            if ((ring_buffer->last_rendered_id == -1 && ring_buffer->max_id > 0) ||
                (ring_buffer->last_rendered_id != -1 &&
                 ring_buffer->max_id - ring_buffer->last_rendered_id > MAX_NUM_AUDIO_FRAMES)) {
                reset_stream(ring_buffer, ring_buffer->max_id);
            }
            break;
        }
        case PACKET_VIDEO: {
            // Skip to the most recent iframe if necessary
            // Bound between max_id and ring_buffer_size, in-case max_id is e.g. 50k and
            // last_renderered_id is 5
            for (int i = ring_buffer->max_id;
                 i >= max(max(0, ring_buffer->last_rendered_id + 1),
                          ring_buffer->max_id - ring_buffer->ring_buffer_size - 10);
                 i--) {
                if (is_ready_to_render(ring_buffer, i)) {
                    FrameData* frame_data = get_frame_at_id(ring_buffer, i);
                    WhistPacket* whist_packet = (WhistPacket*)frame_data->frame_buffer;
                    VideoFrame* video_frame = (VideoFrame*)whist_packet->data;
                    if (video_frame->is_iframe) {
                        LOG_INFO("Catching up to I-Frame at ID %d", i);
                        reset_stream(ring_buffer, i);
                        break;
                    }
                }
            }
            break;
        }
        default: {
            LOG_FATAL("Received a packet that was of unknown type %d!", type);
        }
    }

    // Then, set_rendering the next frame and return the frame_buffer
    int next_to_play_id = ring_buffer->last_rendered_id + 1;
    if (is_ready_to_render(ring_buffer, next_to_play_id)) {
        FrameData* frame = set_rendering(ring_buffer, next_to_play_id);
        // Return the framebuffer that the ringbuffer created
        return frame->frame_buffer;
    } else {
        // no frame ready yet
        return NULL;
    }
}

static void udp_free_packet(void* raw_context, WhistPacket* whist_packet) {
    FATAL_ASSERT(raw_context != NULL);
    UDPContext* context = (UDPContext*)raw_context;

    RingBuffer* ring_buffer = context->ring_buffers[(int)whist_packet->type];

    if (ring_buffer) {
        // There's nothing to free for ringbuffer packets
    } else {
        // If it's not a ringbuffer packet, free it
        free(whist_packet);
    }

    return;
}

static bool udp_get_pending_stream_reset(void* raw_context, WhistPacketType type) {
    FATAL_ASSERT(raw_context != NULL);
    UDPContext* context = (UDPContext*)raw_context;

    if (context->connection_lost) {
        return false;
    }

    if (context->reset_data[type].pending_stream_reset) {
        int greatest_failed_id = context->reset_data[type].greatest_failed_id;
        // We only need to propagate a stream reset once
        context->reset_data[type].pending_stream_reset = false;
        // If it's the start-of-stream,
        // Or we've failed an ID > the last start-of-stream,
        // Then we should propagate the stream reset
        // TODO: Also do this when we haven't received an ACK from the client in a while
        return greatest_failed_id == -1 ||
               greatest_failed_id > context->last_start_of_stream_id[type];
    }
    return false;
}

static void udp_destroy_socket_context(void* raw_context) {
    FATAL_ASSERT(raw_context != NULL);
    UDPContext* context = (UDPContext*)raw_context;

    // Deallocate the nack buffers
    for (int type_id = 0; type_id < NUM_PACKET_TYPES; type_id++) {
        if (context->nack_buffers[type_id] != NULL) {
            for (int i = 0; i < context->nack_num_buffers[type_id]; i++) {
                deallocate_region(context->nack_buffers[type_id][i]);
                free(context->nack_buffer_valid[type_id][i]);
            }
            free(context->nack_buffers[type_id]);
            free(context->nack_buffer_valid[type_id]);
            context->nack_buffers[type_id] = NULL;
        }
    }

    // Destroy the timestamp mutex
    whist_destroy_mutex(context->timestamp_mutex);

    closesocket(context->socket);
    if (context->network_throttler != NULL) {
        network_throttler_destroy(context->network_throttler);
    }
    whist_destroy_mutex(context->mutex);
    free(context);
}

/*
============================
Public Function Implementations
============================
*/

bool create_udp_socket_context(SocketContext* network_context, char* destination, int port,
                               int recvfrom_timeout_ms, int connection_timeout_ms, bool using_stun,
                               char* binary_aes_private_key) {
    // STUN isn't implemented
    FATAL_ASSERT(using_stun == false);

    // Populate function pointer table
    network_context->get_packet = udp_get_packet;
    network_context->socket_update = udp_update;
    network_context->free_packet = udp_free_packet;
    network_context->send_packet = udp_send_packet;
    network_context->get_pending_stream_reset = udp_get_pending_stream_reset;
    network_context->destroy_socket_context = udp_destroy_socket_context;

    // Create the UDPContext, and set to zero
    UDPContext* context = safe_malloc(sizeof(UDPContext));
    memset(context, 0, sizeof(UDPContext));
    // Create the mutex
    context->timestamp_mutex = whist_create_mutex();
    context->last_ping_id = -1;
    context->last_pong_id = -1;
    context->connected = false;
    context->connection_lost = false;
    start_timer(&context->last_network_settings_time);

    network_context->context = context;

    // Map Port
    if ((int)((unsigned short)port) != port) {
        LOG_ERROR("Port invalid: %d", port);
    }
    port = port_mappings[port];
    context->timeout = recvfrom_timeout_ms;
    context->mutex = whist_create_mutex();
    memcpy(context->binary_aes_private_key, binary_aes_private_key,
           sizeof(context->binary_aes_private_key));
    for (int i = 0; i < NUM_PACKET_TYPES; i++) {
        context->reset_data[i].greatest_failed_id = -1;
        context->reset_data[i].pending_stream_reset = true;
        context->fec_packet_ratios[i] = 0.0;
    }

    int ret;
    if (destination == NULL) {
        // On the server, we create a network throttler to limit the
        // outgoing bitrate.
        context->network_throttler =
            network_throttler_create(UDP_NETWORK_THROTTLER_BUCKET_MS, false);
        // Create the server context
        ret = create_udp_server_context(context, port, connection_timeout_ms);
    } else {
        // The client doesn't use a network throttler
        context->network_throttler = NULL;
        // Create the client context
        ret = create_udp_client_context(context, destination, port, connection_timeout_ms);
    }

    if (ret == 0) {
        // Restore the socket's timeout
        set_timeout(context->socket, context->timeout);

        // socket options = TCP_NODELAY IPTOS_LOWDELAY SO_RCVBUF=65536
        // Windows Socket 65535 Socket options apply to all sockets.
        // this is set to stop the kernel from buffering too much, thereby
        // getting the data to us faster for lower latency
        int a = 65535;
        if (setsockopt(context->socket, SOL_SOCKET, SO_RCVBUF, (const char*)&a, sizeof(int)) ==
            -1) {
            LOG_ERROR("Error setting socket opts: %d", get_last_network_error());
            closesocket(context->socket);
            return false;
        }
        return true;
    } else {
        return false;
    }
}

void udp_register_nack_buffer(SocketContext* socket_context, WhistPacketType type,
                              int max_payload_size, int num_buffers) {
    /*
     * Create a nack buffer for the specified packet type which will resend packets to the client in
     * case of data loss.
     */
    UDPContext* context = socket_context->context;

    int type_index = (int)type;
    FATAL_ASSERT(0 <= type_index && type_index < NUM_PACKET_TYPES);
    FATAL_ASSERT(context->nack_buffers[type_index] == NULL);

    // Get max original IDs possible, based off of max payload size and segment size
    int max_original_ids = max_payload_size / MAX_PACKET_SEGMENT_SIZE + 1;
    // Get max FEC ids possible, based on MAX_FEC_RATIO
    int max_fec_ids = get_num_fec_packets(max_original_ids, MAX_FEC_RATIO);

    // Get the max IDs for the transmission
    int max_num_ids = max_original_ids + max_fec_ids;

    // Allocate buffers than can handle the above maximum sizes
    // Memory isn't an issue here, because we'll use our region allocator,
    // so unused memory never gets allocated by the kernel
    context->nack_buffers[type_index] = malloc(sizeof(UDPPacket*) * num_buffers);
    context->nack_buffer_valid[type_index] = malloc(sizeof(bool*) * num_buffers);
    context->nack_mutex[type_index] = whist_create_mutex();
    context->nack_num_buffers[type_index] = num_buffers;
    // This is just used to sanitize the pre-FEC buffer that's passed into send_packet
    context->nack_buffer_max_payload_size[type_index] = max_payload_size;
    context->nack_buffer_max_indices[type_index] = max_num_ids;

    // Allocate each nack buffer, based on num_buffers
    for (int i = 0; i < num_buffers; i++) {
        // Allocate a buffer of max_num_ids WhistPacket's
        context->nack_buffers[type_index][i] = allocate_region(sizeof(UDPPacket) * max_num_ids);
        // Allocate nack buffer validity
        // We hold this separately, since writing anything to the region causes it to allocate
        context->nack_buffer_valid[type_index][i] = malloc(sizeof(bool) * max_num_ids);
        for (int j = 0; j < max_num_ids; j++) {
            context->nack_buffer_valid[type_index][i][j] = false;
        }
    }
}

/*
============================
Questionable Public Function Implementations
============================
*/

int udp_get_num_pending_frames(SocketContext* socket_context, WhistPacketType type) {
    FATAL_ASSERT(socket_context != NULL);
    UDPContext* context = (UDPContext*)socket_context->context;
    FATAL_ASSERT(context != NULL);

    if (context->connection_lost) {
        return 0;
    }

    RingBuffer* ring_buffer = context->ring_buffers[(int)type];

    if (ring_buffer == NULL) {
        // The only pending packet is in the pending packet buffer
        return context->has_pending_packet[(int)type] ? 1 : 0;
    } else {
        // The pending frames are between max_id inclusive and last_rendered_id exclusive
        int max_id = ring_buffer->max_id;
        // All packets are pending
        int num_pending_packets = ring_buffer->max_id - ring_buffer->min_id;
        // But if we've rendered, only frames after the last render ID are pending render
        if (ring_buffer->last_rendered_id != -1) {
            num_pending_packets = ring_buffer->max_id - ring_buffer->last_rendered_id;
        }
        // Min with max packets in the buffer, in-case rendering is way behind,
        // Then the ring_buffer_size is still an upper bound on pending frames
        return min(num_pending_packets, ring_buffer->ring_buffer_size);
    }
}

// TODO: This is weird logic, connecting to higher-level structures
// This should be fixed
int create_udp_listen_socket(SOCKET* sock, int port, int timeout_ms) {
    LOG_INFO("Creating listen UDP Socket");
    *sock = socketp_udp();
    if (*sock == INVALID_SOCKET) {
        LOG_ERROR("Failed to create UDP listen socket");
        return -1;
    }
    set_timeout(*sock, timeout_ms);
    set_tos(*sock, TOS_DSCP_EXPEDITED_FORWARDING);
    // Server connection protocol

    // Bind the server port to the advertized public port
    struct sockaddr_in origin_addr;
    origin_addr.sin_family = AF_INET;
    origin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    origin_addr.sin_port = htons((unsigned short)port);

    if (bind(*sock, (struct sockaddr*)(&origin_addr), sizeof(origin_addr)) < 0) {
        LOG_ERROR("Failed to bind to port %d! errno=%d\n", port, get_last_network_error());
        closesocket(*sock);
        return -1;
    }

    LOG_INFO("Waiting for client to connect to %s:%d...\n", "localhost", port);

    return 0;
}

// TODO: Make this private by making ringbuffers once we notice we should make one
void udp_register_ring_buffer(SocketContext* socket_context, WhistPacketType type,
                              int max_frame_size, int num_buffers) {
    FATAL_ASSERT(socket_context != NULL);
    UDPContext* context = socket_context->context;
    FATAL_ASSERT(context != NULL);

    int type_index = (int)type;
    FATAL_ASSERT(type_index < NUM_PACKET_TYPES);
    FATAL_ASSERT(context->ring_buffers[type_index] == NULL);

    context->ring_buffers[type_index] =
        init_ring_buffer(type, max_frame_size, num_buffers, socket_context, udp_nack_packet,
                         udp_request_stream_reset);
}

NetworkSettings udp_get_network_settings(SocketContext* socket_context) {
    UDPContext* context = (UDPContext*)socket_context->context;

    return context->current_network_settings;
}

// TODO: Pull E2E calculations inside of udp.c
timestamp_us udp_get_client_input_timestamp(SocketContext* socket_context) {
    FATAL_ASSERT(socket_context != NULL);
    UDPContext* context = (UDPContext*)socket_context->context;
    FATAL_ASSERT(context != NULL);

    whist_lock_mutex(context->timestamp_mutex);

    timestamp_us server_timestamp = current_time_us();

    // Theoretical client timestamp of user input, for E2E Latency Calculation
    // The client timestamp from a ping,
    // is the timestamp of theoretical client input we're responding to
    timestamp_us client_input_timestamp = context->last_ping_client_time;
    timestamp_us client_last_server_timestamp = context->last_ping_server_time;
    // But we should adjust for the time between when we received the ping, and now,
    // To only extract the client->server network latency
    client_input_timestamp += (server_timestamp - client_last_server_timestamp);

    whist_unlock_mutex(context->timestamp_mutex);

    return client_input_timestamp;
}

void udp_resend_packet(SocketContext* socket_context, WhistPacketType type, int id, int index) {
    FATAL_ASSERT(socket_context != NULL);
    FATAL_ASSERT(socket_context->context != NULL);
    UDPContext* context = (UDPContext*)socket_context->context;

    if (context->connection_lost) {
        return;
    }

    // Treat this the same as a nack
    udp_handle_nack(context, type, id, index);
}

/*
============================
Private Function Implementation
============================
*/

int create_udp_server_context(UDPContext* context, int port, int connection_timeout_ms) {
    // Track the time we spend in this function, to keep it under connection_timeout_ms
    WhistTimer server_creation_timer;
    start_timer(&server_creation_timer);

    // Create a new listening socket on that port
    create_udp_listen_socket(&context->socket, port, connection_timeout_ms);

    // While we still have some remaining connection time,
    // wait for a connection attempt message from a client
    bool received_connection_attempt = false;
    while ((connection_timeout_ms == -1 ||
            get_timer(&server_creation_timer) * MS_IN_SECOND <= connection_timeout_ms) &&
           !received_connection_attempt) {
        // Make udp_get_udp_packet wait for the remaining time available
        if (connection_timeout_ms == -1) {
            // Propagate blocking
            set_timeout(context->socket, -1);
        } else {
            // We max the remaining time with zero,
            // to prevent accidentally making a blocking -1 call
            set_timeout(
                context->socket,
                max(connection_timeout_ms - get_timer(&server_creation_timer) * MS_IN_SECOND, 0));
        }
        // Check to see if we received a UDP_CONNECTION_ATTEMPT
        UDPPacket client_packet;
        if (udp_get_udp_packet(context, &client_packet)) {
            if (client_packet.type == UDP_CONNECTION_ATTEMPT) {
                received_connection_attempt = true;
            }
        }
    }

    // If a connection attempt wasn't received, cleanup and return -1
    if (!received_connection_attempt) {
        LOG_WARNING("No UDP client was found within the first %d ms", connection_timeout_ms);
        closesocket(context->socket);
        return -1;
    }

    // Connect to that addr, so we can use send instead of sendto
    context->connection_addr = context->last_addr;
    if (connect(context->socket, (struct sockaddr*)&context->connection_addr,
                sizeof(context->connection_addr)) == -1) {
        LOG_WARNING("Failed to connect()!");
        closesocket(context->socket);
        return -1;
    }

    // Send a confirmation message back to the client
    // We send several, as a best attempt against the Two Generals' Problem
    for (int i = 0; i < NUM_CONFIRMATION_MESSAGES; i++) {
        UDPPacket confirmation_packet;
        confirmation_packet.type = UDP_CONNECTION_CONFIRMATION;
        udp_send_udp_packet(context, &confirmation_packet);
    }

    // Connection successful!
    LOG_INFO("Client received on %d from %s:%d over UDP!\n", port,
             inet_ntoa(context->connection_addr.sin_addr),
             ntohs(context->connection_addr.sin_port));

    return 0;
}

int create_udp_client_context(UDPContext* context, char* destination, int port,
                              int connection_timeout_ms) {
    // Track the time we spend in this function, to keep it under connection_timeout_ms
    WhistTimer client_creation_timer;
    start_timer(&client_creation_timer);

    // Create UDP socket
    if ((context->socket = socketp_udp()) == INVALID_SOCKET) {
        return -1;
    }

    // Connect so we can use send instead of sendto
    context->connection_addr.sin_family = AF_INET;
    context->connection_addr.sin_addr.s_addr = inet_addr(destination);
    context->connection_addr.sin_port = htons((unsigned short)port);
    if (connect(context->socket, (const struct sockaddr*)&context->connection_addr,
                sizeof(context->connection_addr)) == -1) {
        LOG_WARNING("Failed to connect()!");
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Connecting to server at %s:%d over UDP...", destination, port);

    // While we still have time left,
    // Keep trying to connect to the server
    bool connection_succeeded = false;
    while ((connection_timeout_ms == -1 ||
            get_timer(&client_creation_timer) * MS_IN_SECOND <= connection_timeout_ms) &&
           !connection_succeeded) {
        // Send a UDP_CONNECTION_ATTEMPT
        UDPPacket client_request;
        client_request.type = UDP_CONNECTION_ATTEMPT;
        udp_send_udp_packet(context, &client_request);

        // Wait for a connection confirmation, using the remaining time available,
        // Up to CONNECTION_ATTEMPT_INTERVAL_MS
        if (connection_timeout_ms == -1) {
            set_timeout(context->socket, CONNECTION_ATTEMPT_INTERVAL_MS);
        } else {
            // Bound between [0, CONNECTION_ATTEMPT_INTERVAL_MS]
            // Don't accidentally call with -1, or it'll block forever
            set_timeout(
                context->socket,
                min(max(connection_timeout_ms - get_timer(&client_creation_timer) * MS_IN_SECOND,
                        0),
                    CONNECTION_ATTEMPT_INTERVAL_MS));
        }
        // Check to see if we received a UDP_CONNECTION_CONFIRMATION
        UDPPacket server_response;
        if (udp_get_udp_packet(context, &server_response)) {
            if (server_response.type == UDP_CONNECTION_CONFIRMATION) {
                connection_succeeded = true;
            }
        }
    }

    if (!connection_succeeded) {
        LOG_WARNING(
            "Connection Attempt Failed. No response was found from the UDP Server within %dms.",
            connection_timeout_ms);
        closesocket(context->socket);
        return -1;
    }

    // Mark as successfully connected
    LOG_INFO("Connected to %s:%d over UDP! (Private %d)\n",
             inet_ntoa(context->connection_addr.sin_addr), port,
             ntohs(context->connection_addr.sin_port));

    return 0;
}

int get_udp_packet_size(UDPPacket* udp_packet) {
    switch (udp_packet->type) {
        case UDP_WHIST_SEGMENT: {
            return offsetof(UDPPacket, udp_whist_segment_data.segment_data) +
                   udp_packet->udp_whist_segment_data.segment_size;
        }
        case UDP_NACK: {
            return offsetof(UDPPacket, udp_nack_data) + sizeof(udp_packet->udp_nack_data);
        }
        case UDP_BITARRAY_NACK: {
            // TODO: Only use bitarray_nack.numBits / 8
            return offsetof(UDPPacket, udp_bitarray_nack_data) +
                   sizeof(udp_packet->udp_bitarray_nack_data);
        }
        case UDP_STREAM_RESET: {
            return offsetof(UDPPacket, udp_stream_reset_data) +
                   sizeof(udp_packet->udp_stream_reset_data);
        }
        case UDP_PING: {
            return offsetof(UDPPacket, udp_ping_data) + sizeof(udp_packet->udp_ping_data);
        }
        case UDP_PONG: {
            return offsetof(UDPPacket, udp_pong_data) + sizeof(udp_packet->udp_pong_data);
        }
        case UDP_NETWORK_SETTINGS: {
            return offsetof(UDPPacket, udp_network_settings_data) +
                   sizeof(udp_packet->udp_network_settings_data);
        }
        case UDP_CONNECTION_ATTEMPT:
        case UDP_CONNECTION_CONFIRMATION: {
            // This should include only the MetaData
            return offsetof(UDPPacket, udp_whist_segment_data);
        }
        default: {
            LOG_FATAL("Unknown UDP Packet Type: %d", udp_packet->type);
        }
    }
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int udp_send_udp_packet(UDPContext* context, UDPPacket* udp_packet) {
    FATAL_ASSERT(context != NULL);
    int udp_packet_size = get_udp_packet_size(udp_packet);

    UDPNetworkPacket udp_network_packet;
    if (ENCRYPTING_PACKETS) {
        // Encrypt the packet during normal operation
        int encrypted_len =
            (int)encrypt_packet(udp_network_packet.payload, &udp_network_packet.aes_metadata,
                                udp_packet, udp_packet_size, context->binary_aes_private_key);
        udp_network_packet.payload_size = encrypted_len;
    } else {
        // Or, just memcpy the segment if ENCRYPTING_PACKETS is disabled
        memcpy(udp_network_packet.payload, udp_packet, udp_packet_size);
        udp_network_packet.payload_size = udp_packet_size;
    }

    // The size of the udp packet that actually needs to be sent over the network
    int udp_network_packet_size = UDPNETWORKPACKET_HEADER_SIZE + udp_network_packet.payload_size;

    // NOTE: This doesn't interfere with clientside hotpath,
    // since the throttler only throttles the serverside
    network_throttler_wait_byte_allocation(context->network_throttler,
                                           (size_t)udp_network_packet_size);

    // If sending fails because of no buffer space available on the system, retry a few times.
    for (int i = 0; i < RETRIES_ON_BUFFER_FULL; i++) {
        // TODO: Remove this mutex? send() is already thread-safe
        whist_lock_mutex(context->mutex);
        int ret;
#if LOG_NETWORKING
        LOG_INFO("Sending a WhistPacket of size %d (Total %d) over UDP", udp_packet_size,
                 udp_network_packet_size);
#endif
        // Send the UDPPacket over the network
        ret = send(context->socket, (const char*)&udp_network_packet,
                   (size_t)udp_network_packet_size, 0);
        whist_unlock_mutex(context->mutex);
        if (ret < 0) {
            int error = get_last_network_error();
            if (error == ENOBUFS) {
                LOG_WARNING("Unexpected UDP Packet Error: %d, retrying to send packet!", error);
                continue;
            } else {
                LOG_WARNING("Unexpected UDP Packet Error: %d", error);
                return -1;
            }
        } else {
            break;
        }
    }

    return 0;
}

static bool udp_get_udp_packet(UDPContext* context, UDPPacket* udp_packet) {
    // Wait to receive a packet over UDP, until timing out
    UDPNetworkPacket udp_network_packet;
    socklen_t slen = sizeof(context->last_addr);
    int recv_len =
        recvfrom_no_intr(context->socket, &udp_network_packet, sizeof(udp_network_packet), 0,
                         (struct sockaddr*)(&context->last_addr), &slen);

    if (connected) {
        // TODO: Compare last_addr, with connection_addr, more accurately than memcmp
        // Not really necessary, since we validate decryption anyway
    }

    // If the packet was successfully received, decrypt and process it it
    if (recv_len > 0) {
        int decrypted_len;

        // Verify the reported packet length
        // This is before the `decrypt_packet` call, so the packet might be malicious
        // ~ We check recv_len against UDPNETWORKPACKET_HEADER_SIZE first, to ensure that
        //  the access to udp_network_packet.{payload_size/aes_metadata} is in-bounds
        // ~ We check bounds on udp_network_packet.payload_size, so that the
        //  the addition check on payload_size doesn't maliciously overflow
        // ~ We make an addition check, to ensure that the payload_size matches recv_len
        if (recv_len < UDPNETWORKPACKET_HEADER_SIZE || udp_network_packet.payload_size < 0 ||
            (int)sizeof(udp_network_packet.payload) < udp_network_packet.payload_size ||
            UDPNETWORKPACKET_HEADER_SIZE + udp_network_packet.payload_size != recv_len) {
            LOG_WARNING("The UDPPacket's payload size %d doesn't agree with recv_len %d!",
                        udp_network_packet.payload_size, recv_len);
            return false;
        }

        if (ENCRYPTING_PACKETS) {
            // Decrypt the packet, into udp_packet
            decrypted_len =
                decrypt_packet(udp_packet, sizeof(UDPPacket), udp_network_packet.aes_metadata,
                               udp_network_packet.payload, udp_network_packet.payload_size,
                               context->binary_aes_private_key);
            // If there was an issue decrypting it, warn and return NULL
            if (decrypted_len < 0) {
                // This is warning, since it could just be someone else sending packets,
                // Not necessarily our fault
                LOG_WARNING("Failed to decrypt packet");
                return false;
            }
            // AFTER THIS LINE,
            // The contents of udp_packet are confirmed to be from the server,
            // And thus can be trusted as not maliciously formed.
        } else {
            // The decrypted packet is just in the payload, during no-encryption dev mode
            decrypted_len = udp_network_packet.payload_size;
            memcpy(udp_packet, udp_network_packet.payload, udp_network_packet.payload_size);
        }
#if LOG_NETWORKING
        LOG_INFO("Received a WhistPacket of size %d over UDP", decrypted_len);
#endif

        // Verify the UDP Packet's size
        FATAL_ASSERT(decrypted_len == get_udp_packet_size(udp_packet));

        return true;
    } else {
        // Network error or no packets to receive
        if (recv_len < 0) {
            int error = get_last_network_error();
            switch (error) {
                case WHIST_ETIMEDOUT:
                case WHIST_EWOULDBLOCK:
                    // Break on expected network errors
                    break;
                default:
                    LOG_WARNING("Unexpected Packet Error: %d", error);
                    break;
            }
        } else {
            // Ignore packets of size 0
        }

        return false;
    }
}

/*
============================
UDP Message Handling
============================
*/

void udp_handle_message(UDPContext* context, UDPPacket* packet) {
    switch (packet->type) {
        case UDP_NACK: {
            // we received a nack from the client, respond
            udp_handle_nack(context, packet->udp_nack_data.whist_type, packet->udp_nack_data.id,
                            packet->udp_nack_data.index);
            break;
        }
        case UDP_BITARRAY_NACK: {
            // nack for everything in the bitarray
            BitArray* bit_arr = bit_array_create(packet->udp_bitarray_nack_data.numBits);
            bit_array_clear_all(bit_arr);

            memcpy(bit_array_get_bits(bit_arr), packet->udp_bitarray_nack_data.ba_raw,
                   BITS_TO_CHARS(packet->udp_bitarray_nack_data.numBits));

            for (int i = packet->udp_bitarray_nack_data.index;
                 i < packet->udp_bitarray_nack_data.numBits; i++) {
                if (bit_array_test_bit(bit_arr, i)) {
                    udp_handle_nack(context, packet->udp_bitarray_nack_data.type,
                                    packet->udp_bitarray_nack_data.id, i);
                }
            }
            bit_array_free(bit_arr);
            break;
        }
        case UDP_PING: {
            udp_handle_ping(context, packet->udp_ping_data.id,
                            packet->udp_ping_data.original_timestamp);
            break;
        }
        case UDP_PONG: {
            udp_handle_pong(context, packet->udp_pong_data.id);
            break;
        }
        case UDP_STREAM_RESET: {
            udp_handle_stream_reset(context, packet->udp_stream_reset_data.whist_type,
                                    packet->udp_stream_reset_data.greatest_failed_id);
            break;
        }
        case UDP_NETWORK_SETTINGS: {
            udp_handle_network_settings(context,
                                        packet->udp_network_settings_data.network_settings);
            break;
        }
        // Ignore handshake packets after the handshake happens
        case UDP_CONNECTION_ATTEMPT:
        case UDP_CONNECTION_CONFIRMATION: {
            break;
        }
        default: {
            LOG_FATAL("Packet of unknown type! %d", (int)packet->type);
        }
    }
}

void udp_handle_nack(UDPContext* context, WhistPacketType type, int packet_id, int packet_index) {
    /*
     * Respond to a client nack by sending the requested packet from the nack buffer if possible.
     */

    int type_index = (int)type;
    FATAL_ASSERT(type_index < NUM_PACKET_TYPES);
    FATAL_ASSERT(context->nack_buffers[type_index] != NULL);
    FATAL_ASSERT(0 <= packet_index);
    // Check that the ID they're asking about could even exist,
    // Note that uninitialized nack_buffer frames have an ID of 0
    FATAL_ASSERT(0 < packet_id);

    if (packet_index >= context->nack_buffer_max_indices[type_index]) {
        // Silence error bc nacking whole frames causes this
        // TODO: Nack for whole frames in a cleaner way
        // LOG_ERROR("Nacked Index %d is >= max indices %d!", packet_index,
        //          context->nack_buffer_max_indices[type_index]);
        return;
    }

    // retrieve the WhistPacket from the nack buffer and send using `udp_send_udp_packet`
    // TODO: change to WhistUDPPacket
    whist_lock_mutex(context->nack_mutex[type_index]);

    // Check if the nack buffer we're looking for is valid
    if (context->nack_buffer_valid[type_index][packet_id % context->nack_num_buffers[type_index]]
                                  [packet_index]) {
        UDPPacket* packet =
            &context->nack_buffers[type_index][packet_id % context->nack_num_buffers[type_index]]
                                  [packet_index];

        // Check that the nack buffer ID's match
        if (packet->udp_whist_segment_data.id == packet_id) {
            packet->udp_whist_segment_data.is_a_nack = true;
            // Wrap in PACKET_VIDEO to prevent verbose audio.c logs
            // TODO: Fix this by making resend_packet not trigger nack logs
            if (type == PACKET_VIDEO) {
                LOG_INFO("NACKed video packet ID %d Index %d found of length %d. Relaying!",
                         packet_id, packet_index, packet->udp_whist_segment_data.segment_size);
            }
            udp_send_udp_packet(context, packet);
        } else {
            if (type == PACKET_VIDEO) {
                LOG_WARNING(
                    "NACKed %s packet %d %d not found, ID %d was "
                    "located instead.",
                    type == PACKET_VIDEO ? "video" : "audio", packet_id, packet_index,
                    packet->udp_whist_segment_data.id);
            }
        }
    } else {
        // TODO: Fix the ability to nack for an entire frame, using something like e.g. index -1
        // This will prevent this log from spamming, particularly on Audio
        if (type == PACKET_VIDEO) {
            LOG_WARNING("NACKed %s packet %d %d not found",
                        type == PACKET_VIDEO ? "video" : "audio", packet_id, packet_index);
        }
    }

    whist_unlock_mutex(context->nack_mutex[type_index]);
}

void udp_handle_stream_reset(UDPContext* context, WhistPacketType type, int greatest_failed_id) {
    context->reset_data[type].greatest_failed_id =
        max(greatest_failed_id, context->reset_data[type].greatest_failed_id);
    context->reset_data[type].pending_stream_reset = true;
}

void udp_update_ping(UDPContext* context) {
    // The ping id we want to send, if any
    int send_ping_id = -1;

    // If we've never ping'ed before
    if (context->last_ping_id == -1) {
        // Mark that we want to send Ping ID 1
        send_ping_id = 1;
    } else {
        // If we've pinged before,
        // we should check for pongs or repings

        // If we've successfully received a pong for the last ping,
        if (context->last_ping_id == context->last_pong_id) {
            // Progress to to the next ping after 500ms since ping start
            if (get_timer(&context->last_new_ping_timer) * MS_IN_SECOND > 500.0) {
                // Mark that we want to send the next ping ID
                send_ping_id = context->last_ping_id + 1;
                // And reset the ping failures counter
                context->consecutive_ping_failures = 0;
            }
        } else {
            // Otherwise, we haven't received the pong yet,
            // and should try to figure out what the problem is

            // If it's been 500ms, give up and mark the failure
            if (get_timer(&context->last_new_ping_timer) * MS_IN_SECOND > 500.0) {
                LOG_WARNING("Ping received no response: %d", context->last_ping_id);
                // Mark that we want to send the next ping now, because we gave up
                send_ping_id = context->last_ping_id + 1;
                // But remember that we've failed
                context->consecutive_ping_failures++;
                // If we've failed too many times consecutively, mark as disconnected
                if (context->consecutive_ping_failures == 4) {
                    LOG_WARNING("Server disconnected: 4 consecutive ping failures.");
                    context->connection_lost = true;
                    send_ping_id = -1;
                }
            }

            // if we haven't received the pong we're waiting for, and it's been 75ms,
            // try sending the ping again. Maybe it was dropped.
            else if (context->last_ping_id != context->last_pong_id &&
                     get_timer(&context->last_ping_timer) * MS_IN_SECOND > 75.0) {
                // Mark that we want to send the same ping ID
                send_ping_id = context->last_ping_id;
            }
        }
    }

    // If we wanted to send a ping with some given ID,
    if (send_ping_id != -1) {
        // Send the ping with that ID
        UDPPacket ping = {0};
        ping.type = UDP_PING;
        ping.udp_ping_data.id = send_ping_id;
        ping.udp_ping_data.original_timestamp = current_time_us();
        if (udp_send_udp_packet(context, &ping) != 0) {
            LOG_WARNING("Failed to send ping! (ID: %d)", send_ping_id);
        }
        // Reset the last ping timer, because we sent a ping
        start_timer(&context->last_ping_timer);
        // If we're sending a new ping ID,
        if (send_ping_id != context->last_ping_id) {
            LOG_INFO("Ping! %d", send_ping_id);
            // Update the last ping ID,
            // and the timer that tracks time since last new ping
            context->last_ping_id = send_ping_id;
            start_timer(&context->last_new_ping_timer);
        }
    }
}

void udp_handle_ping(UDPContext* context, int id, timestamp_us timestamp) {
    // for the server to respond to client pings
    LOG_INFO("Ping Received - Ping ID %d", id);

    // Reply to the ping, with a pong
    UDPPacket pong_packet = {0};
    pong_packet.type = UDP_PONG;
    pong_packet.udp_pong_data.id = id;
    whist_lock_mutex(context->timestamp_mutex);
    context->last_ping_client_time = timestamp;
    context->last_ping_server_time = current_time_us();
    whist_unlock_mutex(context->timestamp_mutex);
    udp_send_udp_packet(context, &pong_packet);
}

void udp_handle_pong(UDPContext* context, int id) {
    // If we've received the pong we're expecting, process it
    if (id == context->last_ping_id) {
        // Only do work if it's a newly received pong
        if (id != context->last_pong_id) {
            double ping_time = get_timer(&context->last_new_ping_timer);
            // TODO: Make this work for client and server
            // log_double_statistic(NETWORK_RTT_UDP, ping_time * MS_IN_SECOND);

            LOG_INFO("Pong %d received: took %f milliseconds", id, ping_time * MS_IN_SECOND);

            // Calculate latency, and mark the last pong
            latency = PING_LAMBDA * latency + (1 - PING_LAMBDA) * ping_time;
            context->last_pong_id = id;
        }
    } else {
        // TODO: Uncomment this FATAL_ASSERT after we have session_id's,
        // since then data will only be from this fresh connection,
        // not an old connection. See #4787 for session_id's
        // FATAL_ASSERT(id < context->last_ping_id);

        // Otherwise, log that we've received an old pong
        LOG_WARNING("Received old pong (ID %d), expected ID %d", id, context->last_ping_id);
    }
}

void udp_handle_network_settings(void* raw_context, NetworkSettings network_settings) {
    UDPContext* context = (UDPContext*)raw_context;
    int burst_bitrate = network_settings.burst_bitrate;
    double audio_fec_ratio = (double)network_settings.audio_fec_ratio;
    double video_fec_ratio = (double)network_settings.video_fec_ratio;

    // Check bounds
    FATAL_ASSERT(0.0 <= audio_fec_ratio && audio_fec_ratio <= MAX_FEC_RATIO);
    FATAL_ASSERT(0.0 <= video_fec_ratio && video_fec_ratio <= MAX_FEC_RATIO);

    // Set burst bitrate, if possible
    if (context->network_throttler == NULL) {
        LOG_ERROR("Tried to set the burst bitrate, but there's no network throttler!");
    } else {
        network_throttler_set_burst_bitrate(context->network_throttler, burst_bitrate);
    }

    // Set FEC Packet Ratios
    context->fec_packet_ratios[PACKET_VIDEO] = video_fec_ratio;
    context->fec_packet_ratios[PACKET_AUDIO] = audio_fec_ratio;

    // Set internal network settings, so that it can be requested for later
    context->current_network_settings = network_settings;
}
