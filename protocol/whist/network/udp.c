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
#include <whist/fec/fec.h>
#include <whist/utils/queue.h>
#include <whist/network/network_algorithm.h>
#include <whist/network/ringbuffer.h>
#include <whist/logging/log_statistic.h>
#include <whist/network/throttle.h>
#include "whist/core/features.h"
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
    // id of the group of packets that are sent in one burst.
    int group_id;
    // The data itself
    union {
        // UDP_WHIST_SEGMENT
        // A segment of a WhistPacket
        WhistSegment udp_whist_segment_data;

        // UDP_NACK
        struct {
            WhistPacketType whist_type;
            int id;
            short index;
        } udp_nack_data;

        // UDP_BITARRAY_NACK
        struct {
            WhistPacketType type;
            int id;
            int index;
            int numBits;
            unsigned char ba_raw[BITS_TO_CHARS(MAX_VIDEO_PACKETS)];
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
            timestamp_us send_timestamp;
        } udp_ping_data;

        // UDP_PONG
        struct {
            int id;
            timestamp_us ping_send_timestamp;
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
// How often to ping
#define UDP_PING_INTERVAL_SEC 0.01
// How long to go without a pong, before the connection is marked as lost
#define UDP_PONG_TIMEOUT_SEC 2.5
// How often to print ping logs
#define UDP_PING_LOG_INTERVAL_SEC 1.0
#define MAX_GROUP_STATS 8
// Incoming bitrate related constants. Choose power-of-two only for an efficient computations
#define INCOMING_BITRATE_WINDOW_MS 1024
#define INCOMING_BITRATE_NUM_BUCKETS 8
#define DURATION_PER_BUCKET (INCOMING_BITRATE_WINDOW_MS / INCOMING_BITRATE_NUM_BUCKETS)
// If video bitrate received over a timeperiod(100ms or more) is less than this value than it will
// be considered as idle video. Actual video stream will consume much more than 500Kbps.
#define IDLE_VIDEO_MAX_BITRATE 500000
// Atleast 75% of the buckets should contain actual video data.
#define MIN_VALID_BUCKETS ((INCOMING_BITRATE_NUM_BUCKETS * 3) / 4)

// Newer out-of-order values will get this weightage in EWMA filter
#define OUT_OF_ORDER_EWMA_FACTOR 0.01
// Number of pings currently in-between the client and server
// Let's choose a nearest power of two, greater than (UDP_PONG_TIMEOUT_SEC / UDP_PING_INTERVAL_SEC)
#define MAX_PINGS_IN_FLIGHT 256

typedef struct {
    bool pending_stream_reset;
    int greatest_failed_id;
} StreamResetData;

typedef struct IncomingBitrate {
    uint64_t bucket_id;
    int num_bits;
} IncomingBitrate;

typedef struct NackID {
    int frame_id;
    int packet_index;
} NackID;

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
    int last_ping_id;
    int last_pong_id;
    WhistTimer ping_timer[MAX_PINGS_IN_FLIGHT];
    WhistTimer last_pong_timer;
    bool connection_lost;
    double long_term_latency;
    double short_term_latency;

    // Latency Calculation (Only used on server)
    WhistMutex timestamp_mutex;
    timestamp_us last_ping_client_time;
    timestamp_us last_ping_server_time;

    UnOrderedPacketInfo unordered_packet_info;

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
    int num_duplicate_packets[NUM_PACKET_TYPES];
    RingBuffer* ring_buffers[NUM_PACKET_TYPES];

    StreamResetData reset_data[NUM_PACKET_TYPES];

    // Holds packets that pending to be received
    WhistPacket pending_packets[NUM_PACKET_TYPES];
    // Whether or not pending_packets[i] contains a pending packet
    bool has_pending_packet[NUM_PACKET_TYPES];

    NetworkSettings network_settings;
    WhistMutex congestion_control_mutex;
    WhistTimer last_network_settings_time;
    // Group related stats and variables required for congestion control
    GroupStats group_stats[MAX_GROUP_STATS];
    int prev_group_id;
    int curr_group_id;
    IncomingBitrate incoming_bitrate_buckets[INCOMING_BITRATE_NUM_BUCKETS];
    void* nack_queue;
} UDPContext;

// Define how many times to retry sending a UDP packet in case of Error 55 (buffer full). The
// current value (5) is an arbitrary choice that was found to work well in practice.
#define RETRIES_ON_BUFFER_FULL 5

// The time in seconds between network statistics requests
#define STATISTICS_SECONDS 5.0
// TODO: Get this out of udp.c somehow
// NOTE that this is matching ./client/audio.c
#define MAX_NUM_AUDIO_FRAMES 10

// The amount to weigh a older pings' latency,
// on the ewma latency value
#define PING_LAMBDA_SHORT_TERM 0.9
#define PING_LAMBDA_LONG_TERM 0.999

// How often should the client send connection attempts
#define CONNECTION_ATTEMPT_INTERVAL_MS 5
// How many confirmation packets the server should respond with,
// When it receives a valid client connection attempt
#define NUM_CONFIRMATION_MESSAGES 10

#define get_bucket_id(time) ((time) / US_IN_MS) / DURATION_PER_BUCKET;

// UDP recv buffer size, when a ringbuffer is being used (1MB)
// This is 450ms-800ms in the 10mbps-18mbps
// This should withstand the highest variance, while still being very small in RAM usage
#define UDP_RECV_BUFFER_SIZE (1 << 20)

// UDP send buffer size, when a network throttler is being used (128KB)
// This is 50ms-100ms in the 10mbps-18mbps range,
// we can safely drop packets if we're truly oversaturating our send bandwidth
#define UDP_SEND_BUFFER_SIZE (1 << 17)

/*
============================
Globals
============================
*/

// TODO: Remove bad globals
extern unsigned short port_mappings[USHRT_MAX + 1];
int override_bitrate;

/*
============================
Private Functions
============================
*/

static void add_incoming_bits(UDPContext* context, timestamp_us arrival_time, int num_bits) {
    uint64_t bucket_id = get_bucket_id(arrival_time);
    IncomingBitrate* bucket =
        &context->incoming_bitrate_buckets[bucket_id % INCOMING_BITRATE_NUM_BUCKETS];
    if (bucket_id > bucket->bucket_id) {
        bucket->bucket_id = bucket_id;
        bucket->num_bits = num_bits;
    } else {
        bucket->num_bits += num_bits;
    }
}

static int get_incoming_bitrate(UDPContext* context) {
    uint64_t current_bucket_id = get_bucket_id(current_time_us());
    int total_bits = 0;
    int num_buckets = 0;
    for (uint64_t i = current_bucket_id - INCOMING_BITRATE_NUM_BUCKETS + 1; i <= current_bucket_id;
         i++) {
        IncomingBitrate* bucket =
            &context->incoming_bitrate_buckets[i % INCOMING_BITRATE_NUM_BUCKETS];
        if (i == bucket->bucket_id) {
            // If the bucket's bitrate is less than IDLE_VIDEO_MAX_BITRATE, then it must have
            // contained idle frames, where video frames would be still. We ignore such idle-video
            // buckets to avoid skewing the average bitrate calculation
            if (bucket->num_bits < (IDLE_VIDEO_MAX_BITRATE * DURATION_PER_BUCKET) / MS_IN_SECOND) {
                continue;
            }
            total_bits += bucket->num_bits;
            num_buckets++;
        }
    }
    // Atleast MIN_VALID_BUCKETS of the buckets should have actual video data
    if (num_buckets >= MIN_VALID_BUCKETS)
        return (int)(((uint64_t)total_bits * MS_IN_SECOND) / (num_buckets * DURATION_PER_BUCKET));
    else
        return -1;
}

// Currently considers only out of order packets within a frame, to keep this logic simpler
// TODO : Consider out-of-order packets across frames as well.
void update_max_unordered_packets(UnOrderedPacketInfo* unordered_info, int frame_id,
                                  int packet_index) {
    // If the current frame is older than previously received, then ignore it.
    if (unordered_info->prev_frame_id > frame_id) {
        return;
    }
    int num_out_of_order_packets = 0;
    if (unordered_info->prev_frame_id == frame_id) {
        num_out_of_order_packets = max(unordered_info->prev_packet_index - packet_index, 0);
    }
    if ((double)num_out_of_order_packets > unordered_info->max_unordered_packets) {
        // Reset the max_unordered_packets if the current value is higher
        unordered_info->max_unordered_packets = (double)num_out_of_order_packets;
    } else {
        // Slowly delay the max_unordered_packets value for improving network conditions
        unordered_info->max_unordered_packets =
            num_out_of_order_packets * OUT_OF_ORDER_EWMA_FACTOR +
            unordered_info->max_unordered_packets * (1.0 - OUT_OF_ORDER_EWMA_FACTOR);
        unordered_info->prev_frame_id = frame_id;
        unordered_info->prev_packet_index = packet_index;
    }
    return;
}

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
 * @param arrival_time           Writes the arrival time of this packet in the pointed location (if
 *                               non-NULL)
 *
 * @param network_payload_size   Writes the payload size of the packet over the network (if
 *                               non-NULL). Valid if the return value of this function is true
 *
 * @returns                      True if a packet was received and written to,
 *                               False if no packet was received
 *
 * @note                         This will call recv on context->socket, which will
 *                               wait for as long as the socket's most recent set_timeout
 */
static bool udp_get_udp_packet(UDPContext* context, UDPPacket* udp_packet,
                               timestamp_us* arrival_time, int* network_payload_size);

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
static void udp_handle_nack(UDPContext* context, WhistPacketType type, int id, int index,
                            bool is_duplicate);
static void udp_handle_ping(UDPContext* context, int id, timestamp_us timestamp);
static void udp_handle_pong(UDPContext* context, int id, timestamp_us ping_send_timestamp);
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

static void send_desired_network_settings(UDPContext* context) {
    if (override_bitrate) {
        context->network_settings.video_bitrate = context->network_settings.burst_bitrate =
            override_bitrate;
    }
    UDPPacket network_settings_packet;
    network_settings_packet.type = UDP_NETWORK_SETTINGS;
    network_settings_packet.udp_network_settings_data.network_settings = context->network_settings;
    udp_send_udp_packet(context, &network_settings_packet);
}

static void udp_congestion_control(UDPContext* context, timestamp_us departure_time,
                                   timestamp_us arrival_time, int group_id) {
    whist_lock_mutex(context->congestion_control_mutex);
    // Initialize desired_network_settings if it is not done yet. Also send that starting bitrate
    // setting to server.
    if (context->network_settings.video_bitrate == 0) {
        context->network_settings = get_starting_network_settings();
        send_desired_network_settings(context);
    }
    bool send_network_settings = false;
    GroupStats* group_stats = &context->group_stats[group_id % MAX_GROUP_STATS];
    // As per WCC.md,
    // An inter-departure time is computed between consecutive groups as T(i) - T(i-1),
    // where T(i) is the departure timestamp of the last packet in the current packet
    // group being processed.  Any packets received out of order are ignored by the
    // arrival-time model.
    if (departure_time > group_stats->departure_time) {
        group_stats->departure_time = departure_time;
        group_stats->arrival_time = arrival_time;
    }
    if (group_id > context->curr_group_id) {
        if (context->prev_group_id != 0) {
            GroupStats* curr_group_stats =
                &context->group_stats[context->curr_group_id % MAX_GROUP_STATS];
            GroupStats* prev_group_stats =
                &context->group_stats[context->prev_group_id % MAX_GROUP_STATS];
            send_network_settings = whist_congestion_controller(
                curr_group_stats, prev_group_stats, get_incoming_bitrate(context),
                get_packet_loss_ratio(context->ring_buffers[PACKET_VIDEO],
                                        context->short_term_latency),
                context->short_term_latency, context->long_term_latency,
                &context->network_settings);
        }
        context->prev_group_id = context->curr_group_id;
        context->curr_group_id = group_id;
    }
    if (send_network_settings) {
        send_desired_network_settings(context);
    }
    whist_unlock_mutex(context->congestion_control_mutex);
}

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
    // Pull a packet from the network, if any is there
    // *************
    static WhistTimer last_recv_timer;
    // Initialize the timer
    if (last_recv_timer.opaque[0] == 0 && last_recv_timer.opaque[1] == 0) {
        start_timer(&last_recv_timer);
    }
    double last_recv = get_timer(&last_recv_timer);
    if (last_recv * MS_IN_SECOND > 5.0) {
        LOG_WARNING_RATE_LIMITED(1, 1, "Time between recv() calls is too long: %fms",
                                 last_recv * MS_IN_SECOND);
    }
    UDPPacket udp_packet;
    timestamp_us arrival_time;
    int network_payload_size;
    bool received_packet =
        udp_get_udp_packet(context, &udp_packet, &arrival_time, &network_payload_size);
    start_timer(&last_recv_timer);

    if (received_packet) {
        // if the packet is a whist_segment, store the data to give later via get_packet
        // Otherwise, pass it to udp_handle_message
        if (udp_packet.type == UDP_WHIST_SEGMENT) {
            WhistPacketType packet_type = udp_packet.udp_whist_segment_data.whist_type;
            if (packet_type == PACKET_VIDEO) {
                add_incoming_bits(context, arrival_time, network_payload_size * BITS_IN_BYTE);
                if (!udp_packet.udp_whist_segment_data.is_a_nack &&
                    !udp_packet.udp_whist_segment_data.is_a_duplicate) {
                    update_max_unordered_packets(&context->unordered_packet_info,
                                                 udp_packet.udp_whist_segment_data.id,
                                                 udp_packet.udp_whist_segment_data.index);
                    if (udp_packet.group_id >= context->curr_group_id) {
                        udp_congestion_control(context,
                                               udp_packet.udp_whist_segment_data.departure_time,
                                               arrival_time, udp_packet.group_id);
                    }
                }
            }
            // If there's a ringbuffer, store in the ringbuffer to reconstruct the original packet
            if (context->ring_buffers[packet_type] != NULL) {
                if (!ring_buffer_receive_segment(context->ring_buffers[packet_type],
                                                 &udp_packet.udp_whist_segment_data)) {
                    // Log when the ringbuffer overflows
                    LOG_ERROR("Ringbuffer overflowed; stream resets have been failing to recover.");
                    // Optionally mark the connection has lost during such an event
                    // context->connection_lost = true;
                }
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

    if (context->ring_buffers[PACKET_VIDEO] != NULL) {
        try_recovering_missing_packets_or_frames(
            context->ring_buffers[PACKET_VIDEO], context->short_term_latency,
            (int)round(context->unordered_packet_info.max_unordered_packets),
            &context->network_settings);
    }

    return true;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
// Don't call this function in hotpath for video packets, as it can wait in throttle.
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
    int num_indices_if_no_fec =
        (whist_packet_size == 0 ? 1 : int_div_roundup(whist_packet_size, MAX_PACKET_SEGMENT_SIZE));
    int num_indices_if_use_fec =
        fec_encoder_get_num_real_buffers(whist_packet_size, MAX_PACKET_SEGMENT_SIZE);

    // Calculate the number of FEC packets we'll be using, if any
    // A nack buffer is required to use FEC
    int num_fec_packets = 0;
    double fec_packet_ratio = context->fec_packet_ratios[packet_type];
    if (nack_buffer && fec_packet_ratio > 0.0) {
        num_fec_packets = get_num_fec_packets(num_indices_if_use_fec, fec_packet_ratio);
    }

    int num_indices;
    if (num_fec_packets == 0) {
        num_indices = num_indices_if_no_fec;
    } else {
        num_indices = num_indices_if_use_fec;
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

        WhistTimer encode_timer;
        start_timer(&encode_timer);
        // If using FEC, populate the UDP payload buffers with the FEC encoded buffers
        fec_get_encoded_buffers(fec_encoder, (void**)buffers, buffer_sizes);
        double encode_time = get_timer(&encode_timer) * MS_IN_SECOND;
        if (LOG_FEC_ENCODE) {
            LOG_INFO("[FEC] encoded %d original + %d redundant buffers, in %f ms", num_indices,
                     num_fec_packets, encode_time);
        }
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

    int prev_frame_num_duplicates = context->num_duplicate_packets[packet_type];
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
        packet->udp_whist_segment_data.prev_frame_num_duplicates =
            (unsigned short)prev_frame_num_duplicates;
        packet->udp_whist_segment_data.is_a_nack = false;
        packet->udp_whist_segment_data.is_a_duplicate = false;
        packet->udp_whist_segment_data.segment_size = buffer_sizes[packet_index];

        FATAL_ASSERT(packet->udp_whist_segment_data.segment_size <=
                     sizeof(packet->udp_whist_segment_data.segment_data));
        memcpy(packet->udp_whist_segment_data.segment_data, buffers[packet_index],
               buffer_sizes[packet_index]);

        // Before sending the video packets for current frame, handle any nack requests for
        // previous frames.
        if (packet_type == PACKET_VIDEO) {
            udp_handle_pending_nacks(raw_context);
        }

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
            if (ring_buffer->last_rendered_id == -1) {
                // INITIAL_FRAMES is the maximum number of frames
                // that the ringbuffer will start with
#define INITIAL_FRAMES 6
                // If the max_id is too large, just skip to closely behind it
                if (ring_buffer->max_id > INITIAL_FRAMES) {
                    // Will start off the stream with INITIAL_FRAMES in the buffer
                    reset_stream(ring_buffer, ring_buffer->max_id - INITIAL_FRAMES + 1);
                }
            } else {
                int next_to_render_id = ring_buffer->last_rendered_id + 1;
                // If we're head-of-line blocking,
                if (!is_ready_to_render(ring_buffer, next_to_render_id)) {
                    // Find the first renderable frame between [next-to-render-id+1, and max-id]
                    int start_checking =
                        max(next_to_render_id + 1,
                            ring_buffer->max_id - ring_buffer->ring_buffer_size + 1);
                    for (int id = start_checking; id <= ring_buffer->max_id; id++) {
                        // And skip to that renderable frame, if max_id >= X+2, or it's 25ms old
                        if (is_ready_to_render(ring_buffer, id)) {
                            FrameData* frame_data = get_frame_at_id(ring_buffer, id);
                            if (ring_buffer->max_id >= id + 2 ||
                                get_timer(&frame_data->frame_creation_timer) * MS_IN_SECOND >
                                    25.0) {
                                reset_stream(ring_buffer, id);
                            }
                            // We only care about the first renderable frame
                            break;
                        }
                    }
                }
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
                    if (VIDEO_FRAME_TYPE_IS_RECOVERY_POINT(video_frame->frame_type)) {
                        LOG_INFO("Catching up to recovery point (type %d) at ID %d",
                                 video_frame->frame_type, i);
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
            whist_destroy_mutex(context->nack_mutex[type_id]);
            context->nack_buffers[type_id] = NULL;
        }
    }

    // Destroy ring buffers, if present.
    for (int type_id = 0; type_id < NUM_PACKET_TYPES; type_id++) {
        if (context->ring_buffers[type_id]) {
            destroy_ring_buffer(context->ring_buffers[type_id]);
            context->ring_buffers[type_id] = NULL;
        }
    }

    // Destroy the mutexes
    whist_destroy_mutex(context->timestamp_mutex);
    whist_destroy_mutex(context->congestion_control_mutex);

    closesocket(context->socket);
    if (context->network_throttler != NULL) {
        network_throttler_destroy(context->network_throttler);
    }
    if (context->nack_queue != NULL) {
        fifo_queue_destroy(context->nack_queue);
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

    // Create the UDPContext, and set to zero
    UDPContext* context = safe_malloc(sizeof(UDPContext));
    memset(context, 0, sizeof(UDPContext));
    // Create the mutex
    context->timestamp_mutex = whist_create_mutex();
    context->congestion_control_mutex = whist_create_mutex();
    context->last_ping_id = -1;
    context->last_pong_id = -1;
    // Whether or not we've ever connected
    context->connected = false;
    // Whether or not we've connected, but then lost the connection
    context->connection_lost = false;
    context->unordered_packet_info.max_unordered_packets = 0.0;
    start_timer(&context->last_network_settings_time);

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
        // Create the server context
        ret = create_udp_server_context(context, port, connection_timeout_ms);
        if (ret == 0) {
            // On the server, we create a network throttler to limit the
            // outgoing bitrate.
            context->network_throttler =
                network_throttler_create(UDP_NETWORK_THROTTLER_BUCKET_MS, true);
            // When creating a network throttler to throttle high-bandwidth,
            // we also want to ensure the send buffer size is large enough
            int a = UDP_SEND_BUFFER_SIZE;
            if (setsockopt(context->socket, SOL_SOCKET, SO_SNDBUF, (const char*)&a, sizeof(int)) ==
                -1) {
                LOG_ERROR("Error setting socket opt: %d", get_last_network_error());
            }
        }
    } else {
        // The client doesn't use a network throttler
        context->network_throttler = NULL;
        // Create the client context
        ret = create_udp_client_context(context, destination, port, connection_timeout_ms);
    }

    if (ret == 0) {
        // Populate function pointer table
        network_context->context = context;
        network_context->get_packet = udp_get_packet;
        network_context->socket_update = udp_update;
        network_context->free_packet = udp_free_packet;
        network_context->send_packet = udp_send_packet;
        network_context->get_pending_stream_reset = udp_get_pending_stream_reset;
        network_context->destroy_socket_context = udp_destroy_socket_context;
        // Mark as connected
        context->connected = true;
        // Restore the socket's timeout
        set_timeout(context->socket, context->timeout);
        return true;
    } else {
        memset(network_context, 0, sizeof(*network_context));
        free(context);
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
        // All frames are pending,
        int num_pending_frames = ring_buffer->max_id - ring_buffer->min_id + 1;
        // But if we've rendered, only frames after the last render ID are pending render
        if (ring_buffer->last_rendered_id != -1) {
            num_pending_frames = ring_buffer->max_id - ring_buffer->last_rendered_id;
        }
        // Min with max possible frames in the buffer. If rendering is way behind,
        // Then the ring_buffer_size is still an upper bound on pending frames
        return min(num_pending_frames, ring_buffer->ring_buffer_size);
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

    // We'll want to increase the UDP buffer size,
    // when we know we may be accepting high-volume packets
    int a = UDP_RECV_BUFFER_SIZE;
    if (setsockopt(context->socket, SOL_SOCKET, SO_RCVBUF, (const char*)&a, sizeof(int)) == -1) {
        LOG_ERROR("Error setting socket opt: %d", get_last_network_error());
    }
}

NetworkSettings udp_get_network_settings(SocketContext* socket_context) {
    UDPContext* context = (UDPContext*)socket_context->context;

    return context->network_settings;
}

// TODO: Pull E2E calculations inside of udp.c
timestamp_us udp_get_client_input_timestamp(SocketContext* socket_context) {
    FATAL_ASSERT(socket_context != NULL);
    UDPContext* context = (UDPContext*)socket_context->context;
    FATAL_ASSERT(context != NULL);

    // Not even one ping is received. So send '0' to indicate a null/invalid client timestamp
    if (context->last_ping_client_time == 0 || context->last_ping_server_time == 0) return 0;

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

    context->num_duplicate_packets[type]++;
    // Treat this the same as a nack, but set duplicate flag as true
    udp_handle_nack(context, type, id, index, true);
}

void udp_reset_duplicate_packet_counter(SocketContext* socket_context, WhistPacketType type) {
    UDPContext* context = (UDPContext*)socket_context->context;
    context->num_duplicate_packets[type] = 0;
}

int udp_get_num_indices(SocketContext* socket_context, WhistPacketType type, int packet_id) {
    UDPContext* context = (UDPContext*)socket_context->context;
    UDPPacket* packet =
        &context->nack_buffers[type][packet_id % context->nack_num_buffers[type]][0];
    if (packet->udp_whist_segment_data.id == packet_id) {
        return packet->udp_whist_segment_data.num_indices;
    } else {
        LOG_WARNING("%s packet %d not found, ID %d was located instead.",
                    type == PACKET_VIDEO ? "video" : "audio", packet_id,
                    packet->udp_whist_segment_data.id);
        return -1;
    }
}

bool udp_handle_pending_nacks(void* raw_context) {
    UDPContext* context = (UDPContext*)raw_context;
    NackID nack_id;
    bool ret = false;
    while (fifo_queue_dequeue_item(context->nack_queue, &nack_id) != -1) {
        udp_handle_nack(context, PACKET_VIDEO, nack_id.frame_id, nack_id.packet_index, false);
        ret = true;
    }
    return ret;
}

void udp_handle_resize(SocketContext* socket_context, int dpi) {
    UDPContext* context = (UDPContext*)socket_context->context;
    if (context == NULL) {
        return;
    }
    whist_lock_mutex(context->congestion_control_mutex);
    network_algo_set_dpi(dpi);
    NetworkSettings starting_network_settings = get_starting_network_settings();
    // If the current bitrate is lesser than the starting bitrate of the new screen, use the
    // new starting bitrate to avoid pixelation
    if (context->network_settings.video_bitrate < starting_network_settings.video_bitrate) {
        context->network_settings = starting_network_settings;
        send_desired_network_settings(context);
    }
    whist_unlock_mutex(context->congestion_control_mutex);
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
    if (create_udp_listen_socket(&context->socket, port, connection_timeout_ms)) {
        LOG_ERROR("Failed to create UDP listen socket");
        return -1;
    }

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
        if (udp_get_udp_packet(context, &client_packet, NULL, NULL)) {
            if (client_packet.type == UDP_CONNECTION_ATTEMPT) {
                received_connection_attempt = true;
            }
        }
    }

    // If a connection attempt wasn't received, cleanup and return -1
    if (!received_connection_attempt) {
        LOG_INFO("No UDP client was found within the first %d ms", connection_timeout_ms);
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
    context->nack_queue =
        fifo_queue_create(sizeof(NackID), VIDEO_NACKBUFFER_SIZE * MAX_VIDEO_PACKETS);

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
        if (udp_get_udp_packet(context, &server_response, NULL, NULL)) {
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
// Don't call this function in hotpath for video packets, as it can wait in throttle.
int udp_send_udp_packet(UDPContext* context, UDPPacket* udp_packet) {
    FATAL_ASSERT(context != NULL);
    int udp_packet_size = get_udp_packet_size(udp_packet);
    bool throttle = false;
    if (udp_packet->type == UDP_WHIST_SEGMENT) {
        udp_packet->udp_whist_segment_data.departure_time = current_time_us();
        // Throttle only video packet. Audio packets are very small and run on reserved bandwidth
        // and ping/pong packets use negligible bandwidth.
        if (udp_packet->udp_whist_segment_data.whist_type == PACKET_VIDEO) {
            throttle = true;
        }
    }

    // NOTE: This doesn't interfere with clientside hotpath,
    // since the throttler only throttles the serverside
    if (throttle) {
        udp_packet->group_id = network_throttler_wait_byte_allocation(
            context->network_throttler, (size_t)(UDPNETWORKPACKET_HEADER_SIZE + udp_packet_size));
    }

    UDPNetworkPacket udp_network_packet;
    if (FEATURE_ENABLED(PACKET_ENCRYPTION)) {
        // Encrypt the packet during normal operation
        int encrypted_len =
            (int)encrypt_packet(udp_network_packet.payload, &udp_network_packet.aes_metadata,
                                udp_packet, udp_packet_size, context->binary_aes_private_key);
        udp_network_packet.payload_size = encrypted_len;
    } else {
        // Or, just memcpy the segment if PACKET_ENCRYPTION is disabled
        memcpy(udp_network_packet.payload, udp_packet, udp_packet_size);
        udp_network_packet.payload_size = udp_packet_size;
    }

    // The size of the udp packet that actually needs to be sent over the network
    int udp_network_packet_size = UDPNETWORKPACKET_HEADER_SIZE + udp_network_packet.payload_size;

    // If sending fails because of no buffer space available on the system, retry a few times.
    for (int i = 0; i < RETRIES_ON_BUFFER_FULL; i++) {
        // TODO: Remove this mutex? send() is already thread-safe
        whist_lock_mutex(context->mutex);
        int ret;
        if (LOG_NETWORKING) {
            LOG_INFO("Sending a WhistPacket of size %d (Total %d) over UDP", udp_packet_size,
                     udp_network_packet_size);
        }
        // Send the UDPPacket over the network
        ret = send(context->socket, (const char*)&udp_network_packet,
                   (size_t)udp_network_packet_size, 0);
        whist_unlock_mutex(context->mutex);
        if (ret < 0) {
            int error = get_last_network_error();
            if (error == WHIST_ECONNREFUSED) {
                if (context->connected) {
                    // The connection has been lost
                    LOG_WARNING("UDP connection Lost: ECONNREFUSED");
                    context->connection_lost = true;
                    return -1;
                }
                break;
            } else if (error == WHIST_ENOBUFS) {
                LOG_WARNING("Unexpected UDP Packet Error: %d (Retrying to send packet!)", error);
                continue;
            } else {
                LOG_WARNING("Unexpected UDP Packet Error: %d", error);
                return -1;
            }
        } else {
            break;
        }
    }

    // If encryption has added any extra bytes due to padding, then network throttler should be
    // called again to adjust for these extra bytes, so that the requested bitrate limit is not
    // exceeded.
    if (throttle && udp_network_packet.payload_size > udp_packet_size) {
        network_throttler_wait_byte_allocation(context->network_throttler,
                                               udp_network_packet.payload_size - udp_packet_size);
    }
    return 0;
}

static bool udp_get_udp_packet(UDPContext* context, UDPPacket* udp_packet,
                               timestamp_us* arrival_time, int* network_payload_size) {
    // Wait to receive a packet over UDP, until timing out
    UDPNetworkPacket udp_network_packet;
    socklen_t slen = sizeof(context->last_addr);
    int recv_len =
        recvfrom_no_intr(context->socket, &udp_network_packet, sizeof(udp_network_packet), 0,
                         (struct sockaddr*)(&context->last_addr), &slen);

    if (context->connected) {
        // TODO: Compare last_addr, with connection_addr, more accurately than memcmp
        // Not strictly necessary, since we validate decryption anyway
    }

    // If the packet was successfully received, decrypt and process it it
    if (recv_len > 0) {
        int decrypted_len;

        // Tracks arrival time for congestion control algo
        if (arrival_time) {
            *arrival_time = current_time_us();
        }

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

        if (FEATURE_ENABLED(PACKET_ENCRYPTION)) {
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
        if (LOG_NETWORKING) {
            LOG_INFO("Received a WhistPacket of size %d over UDP", decrypted_len);
        }
        if (network_payload_size) {
            *network_payload_size =
                (UDPNETWORKPACKET_HEADER_SIZE + udp_network_packet.payload_size);
        }

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
                case WHIST_ECONNREFUSED: {
                    if (context->connected) {
                        // The connection has been lost
                        LOG_WARNING("UDP connection Lost: ECONNREFUSED");
                        context->connection_lost = true;
                    }
                    break;
                }
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
            FATAL_ASSERT(packet->udp_nack_data.whist_type == PACKET_VIDEO);
            NackID nack_id;
            nack_id.frame_id = packet->udp_nack_data.id;
            if ((short)packet->udp_nack_data.index >= 0) {
                nack_id.packet_index = packet->udp_nack_data.index;
                if (fifo_queue_enqueue_item(context->nack_queue, &nack_id) < 0) {
                    LOG_ERROR("Failed to enqueue NACK request");
                }
            } else {
                // NACK for all packets in a frame when index is negative
                whist_lock_mutex(context->nack_mutex[PACKET_VIDEO]);
                UDPPacket* first_packet =
                    &context
                         ->nack_buffers[PACKET_VIDEO][nack_id.frame_id %
                                                      context->nack_num_buffers[PACKET_VIDEO]][0];
                if (first_packet->udp_whist_segment_data.id != nack_id.frame_id) {
                    LOG_WARNING("NACKed video packet %d not found, ID %d was located instead.",
                                nack_id.frame_id, first_packet->udp_whist_segment_data.id);
                    whist_unlock_mutex(context->nack_mutex[PACKET_VIDEO]);
                    break;
                }
                for (int i = 0; i < first_packet->udp_whist_segment_data.num_indices; i++) {
                    nack_id.packet_index = i;
                    if (LOG_NACKING) {
                        LOG_INFO("Generating Nack for Frame ID %d, index %d", nack_id.frame_id, i);
                    }
                    if (fifo_queue_enqueue_item(context->nack_queue, &nack_id) < 0) {
                        LOG_ERROR("Failed to enqueue NACK request");
                    }
                }
                whist_unlock_mutex(context->nack_mutex[PACKET_VIDEO]);
            }
            break;
        }
        case UDP_BITARRAY_NACK: {
            // nack for everything in the bitarray
            BitArray* bit_arr = bit_array_create(packet->udp_bitarray_nack_data.numBits);
            bit_array_clear_all(bit_arr);

            memcpy(bit_array_get_bits(bit_arr), packet->udp_bitarray_nack_data.ba_raw,
                   BITS_TO_CHARS(packet->udp_bitarray_nack_data.numBits));

            FATAL_ASSERT(packet->udp_bitarray_nack_data.type == PACKET_VIDEO);
            NackID nack_id;
            nack_id.frame_id = packet->udp_bitarray_nack_data.id;
            for (int i = packet->udp_bitarray_nack_data.index;
                 i < packet->udp_bitarray_nack_data.numBits; i++) {
                if (bit_array_test_bit(bit_arr, i)) {
                    nack_id.packet_index = i;
                    if (fifo_queue_enqueue_item(context->nack_queue, &nack_id) < 0) {
                        LOG_ERROR("Failed to enqueue NACK request");
                    }
                }
            }
            bit_array_free(bit_arr);
            break;
        }
        case UDP_PING: {
            udp_handle_ping(context, packet->udp_ping_data.id,
                            packet->udp_ping_data.send_timestamp);
            break;
        }
        case UDP_PONG: {
            udp_handle_pong(context, packet->udp_pong_data.id,
                            packet->udp_pong_data.ping_send_timestamp);
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

void udp_handle_nack(UDPContext* context, WhistPacketType type, int packet_id, int packet_index,
                     bool is_duplicate) {
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
            packet->udp_whist_segment_data.is_a_nack = !is_duplicate;
            packet->udp_whist_segment_data.is_a_duplicate = is_duplicate;
            // Wrap in PACKET_VIDEO to prevent verbose audio.c logs
            // TODO: Fix this by making resend_packet not trigger nack logs
            if (LOG_NACKING && type == PACKET_VIDEO && !is_duplicate) {
                LOG_INFO("NACKed video packet ID %d Index %d found of length %d. Relaying!",
                         packet_id, packet_index, packet->udp_whist_segment_data.segment_size);
            }
            udp_send_udp_packet(context, packet);
        } else {
            // TODO: Calculate an aggregate and LOG_WARNING that,
            // Insteads of per-packet logging
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
        if (LOG_NACKING && type == PACKET_VIDEO) {
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
        start_timer(&context->last_pong_timer);
    } else {
        // If we've pinged before,
        // we should check for pongs or repings

        // If it's been too long since the last pong, give up and mark the connection as lost
        if (get_timer(&context->last_pong_timer) > UDP_PONG_TIMEOUT_SEC) {
            LOG_WARNING("Server disconnected: No pong response received for %.2f seconds",
                        get_timer(&context->last_pong_timer));
            context->connection_lost = true;
        }
        // Progress to to the next ping after UDP_PING_INTERVAL_SEC
        else if (get_timer(&context->ping_timer[context->last_ping_id % MAX_PINGS_IN_FLIGHT]) >
                 UDP_PING_INTERVAL_SEC) {
            // Mark that we want to send the next ping ID
            send_ping_id = context->last_ping_id + 1;
        }
    }

    // If we wanted to send a ping with some given ID,
    if (send_ping_id != -1) {
        // Send the ping with that ID
        UDPPacket ping = {0};
        ping.type = UDP_PING;
        ping.udp_ping_data.id = send_ping_id;
        ping.udp_ping_data.send_timestamp = current_time_us();
        if (udp_send_udp_packet(context, &ping) != 0) {
            LOG_WARNING("Failed to send ping! (ID: %d)", send_ping_id);
        }
        // Reset the last ping timer, because we sent a ping
        start_timer(&context->ping_timer[send_ping_id % MAX_PINGS_IN_FLIGHT]);
#if LOG_VIDEO || LOG_AUDIO || LOG_NETWORKING
        LOG_INFO("Ping! %d", send_ping_id);
#endif
        // Update the last ping ID,
        context->last_ping_id = send_ping_id;
    }
}

void udp_handle_ping(UDPContext* context, int id, timestamp_us timestamp) {
#if LOG_VIDEO || LOG_AUDIO || LOG_NETWORKING
    LOG_INFO("Ping Received - Ping ID %d", id);
#endif

    // Reply to the ping, with a pong
    UDPPacket pong_packet = {0};
    pong_packet.type = UDP_PONG;
    pong_packet.udp_pong_data.id = id;
    pong_packet.udp_pong_data.ping_send_timestamp = timestamp;
    whist_lock_mutex(context->timestamp_mutex);
    context->last_ping_client_time = timestamp;
    context->last_ping_server_time = current_time_us();
    whist_unlock_mutex(context->timestamp_mutex);
    udp_send_udp_packet(context, &pong_packet);
}

void udp_handle_pong(UDPContext* context, int id, timestamp_us ping_send_timestamp) {
    start_timer(&context->last_pong_timer);

    double ping_time = (current_time_us() - ping_send_timestamp) / (double)US_IN_SECOND;
    // Initialize the latency
    if (context->last_pong_id == -1) {
        context->short_term_latency = context->long_term_latency = ping_time;
    }
    context->last_pong_id = id;

    log_double_statistic(NETWORK_RTT_UDP, ping_time * MS_IN_SECOND);
    // Comment logs once every second, or when latency spikes
    if (LOG_VIDEO || LOG_AUDIO || LOG_NETWORKING ||
        id % max((int)(UDP_PING_LOG_INTERVAL_SEC / (double)UDP_PING_INTERVAL_SEC), 1) == 0 ||
        ping_time > 2.0 * context->long_term_latency) {
        LOG_INFO_RATE_LIMITED(
            1, 5,
            "Pong %d received: took %.2fms, long term latency %.2fms, short term latency %.2fms",
            id, ping_time * MS_IN_SECOND, context->long_term_latency * MS_IN_SECOND,
            context->short_term_latency * MS_IN_SECOND);
    }

    // Calculate latency
    context->short_term_latency = PING_LAMBDA_SHORT_TERM * context->short_term_latency +
                                  (1 - PING_LAMBDA_SHORT_TERM) * ping_time;
    // Don't update long term latency during congestion
    if (!context->network_settings.congestion_detected) {
        context->long_term_latency = PING_LAMBDA_LONG_TERM * context->long_term_latency +
                                     (1 - PING_LAMBDA_LONG_TERM) * ping_time;
    }
}

void udp_handle_network_settings(void* raw_context, NetworkSettings network_settings) {
    UDPContext* context = (UDPContext*)raw_context;
    int burst_bitrate = network_settings.burst_bitrate;

    // double audio_fec_ratio = (double)network_settings.audio_fec_ratio;
    // double video_fec_ratio = (double)network_settings.video_fec_ratio;

    // temp fix as the FEC parameter synchronization mechanism is broken by WCC
    // TODO: a better fix
    double audio_fec_ratio = AUDIO_FEC_RATIO;
    double video_fec_ratio = VIDEO_FEC_RATIO;

    // Check bounds
    FATAL_ASSERT(0.0 <= audio_fec_ratio && audio_fec_ratio <= MAX_FEC_RATIO);
    FATAL_ASSERT(0.0 <= video_fec_ratio && video_fec_ratio <= MAX_FEC_RATIO);

    // Set burst bitrate, if possible
    if (context->network_throttler == NULL) {
        LOG_ERROR("Tried to set the burst bitrate, but there's no network throttler!");
    } else {
        // Subtract audio bitrate from burst bitrate as audio is not throttled
        network_throttler_set_burst_bitrate(context->network_throttler, burst_bitrate);
    }

    // Set FEC Packet Ratios
    context->fec_packet_ratios[PACKET_VIDEO] = video_fec_ratio;
    context->fec_packet_ratios[PACKET_AUDIO] = audio_fec_ratio;

    // Set internal network settings, so that it can be requested for later
    context->network_settings = network_settings;
}

size_t udp_packet_max_size(void) { return (sizeof(UDPNetworkPacket)); }
