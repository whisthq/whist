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
#include <whist/network/ringbuffer.h>

#ifndef _WIN32
#include <fcntl.h>
#endif

/*
============================
Defines
============================
*/

// We should be able to fix a normal-sized WhistPacket,
// in just a single segment
#define MAX_PACKET_SEGMENT_SIZE ((int)sizeof(WhistPacket))

typedef enum {
    UDP_WHIST_SEGMENT,
    UDP_NACK,
    UDP_BITARRAY_NACK,
    UDP_STREAM_RESET,
    UDP_PING,
    UDP_NETWORK_SETTINGS,
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
        struct {
            WhistPacketType whist_type;
            int id;
            unsigned short index;
            unsigned short num_indices;
            unsigned short num_fec_indices;
            unsigned short segment_size;
            bool is_a_nack;
            // Must be last, since only the first segment_size bytes will be sent
            char segment_data[MAX_PACKET_SEGMENT_SIZE];
        } udp_whist_segment_data;

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
#define UDPNETWORKPACKET_HEADER_SIZE ((int)sizeof(UDPNetworkPacket) - (int)sizeof((UDPNetworkPacket){0}.payload))

struct StreamResetData {
    bool pending_stream_reset;
    int greatest_failed_id;
};

// An instance of the UDP Context
typedef struct {
    int timeout;
    SOCKET socket;
    struct sockaddr_in addr;
    int ack;
    WhistMutex mutex;
    char binary_aes_private_key[16];
    NetworkThrottleContext* network_throttler;

    double fec_packet_ratios[NUM_PACKET_TYPES];

    // Nack Buffer Data
    UDPPacket** nack_buffers[NUM_PACKET_TYPES];
    // This mutex will protect the data in nack_buffers
    WhistMutex nack_mutex[NUM_PACKET_TYPES];
    int nack_num_buffers[NUM_PACKET_TYPES];
    int nack_buffer_max_indices[NUM_PACKET_TYPES];
    int nack_buffer_max_payload_size[NUM_PACKET_TYPES];
    RingBuffer* ring_buffers[NUM_PACKET_TYPES];
    // because we don't need to buffer messages
    WhistPacket last_packets[NUM_PACKET_TYPES];
    // Stream reset data for each type
    StreamResetData reset_data[NUM_PACKET_TYPES];
} UDPContext;

// Define how many times to retry sending a UDP packet in case of Error 55 (buffer full). The
// current value (5) is an arbitrary choice that was found to work well in practice.
#define RETRIES_ON_BUFFER_FULL 5

// Define the bitrate specified to be maintained for each and every 0.5 ms internal.
#define UDP_NETWORK_THROTTLER_BUCKET_MS 0.5
// TODO: make this something sync_packets knows about, not udp.c
#define MAX_NUM_AUDIO_FRAMES 8

/*
============================
Globals
============================
*/

// TODO: Remove
extern unsigned short port_mappings[USHRT_MAX + 1];
extern volatile double latency;

/*
============================
Private Functions
============================
*/

/**
 * @brief                        Encrypts and Sends a payload of size at most MAX_UDP_PAYLOAD_SIZE through the network.
 */
static int udp_send_udp_packet(UDPContext* context, UDPPacket* udp_packet);

/**
 * @brief                          Respond to a nack for a given ID/Index
 *                                 NOTE: This function is thread-safe with send_packet
 *
 * @param context                  The SocketContext to nack from
 * @param type                     The WhistPacketType of the nack'ed packet
 * @param id                       The ID of the nack'ed packet
 * @param index                    The index of the nack'ed packet
 *                                 (The UDP packet index into the larger WhistPacket)
 */
static int udp_nack(SocketContext* context, WhistPacketType type, int id, int index);

/**
 * @brief                   Handle any UDPPacket that's not a Whist segment, e.g. nack messages and network setting updates
 *
 * @param context           The UDPContext to handle the message
 * @param packet            The UDPPacket to handle
 */
static int udp_handle_message(UDPContext* context, UDPPacket* packet);

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

/**
 * @brief                          Send a 0-length packet over the socket. Used
 *                                 to keep-alive over NATs, and to check on the
 *                                 validity of the socket
 *
 * @param context                  The socket context
 *
 * @returns                        Will return -1 on failure, and 0 on success
 *                                 Failure implies that the socket is
 *                                 broken or the TCP connection has ended, use
 *                                 get_last_network_error() to learn more about the
 *                                 error
 */
static int udp_ack(UDPContext* context);

/*
============================
UDP Implementation of Network.h Interface
============================
*/

void udp_update(void* raw_context, bool should_recv) {
    /*
     * Read a WhistPacket from the socket, decrypt it if necessary, and store the decrypted data for the next get_packet call.
     * TODO: specify that we are reading WhistUDPPackets from the UDP socket and handle combining the WhistUDPPackets into WhistPackets. Return NULL if no full packet is available, and the full WhistPacket otherwise.
     * TODO: handle nacking by creating ring buffers for packet types that require nacking (audio and video)
     */ 
    UDPContext* context = raw_context;

    // Keeps the connection alive
    udp_ack(context);

    // first, try recovering packets
    for (int i = 0; i < NUM_PACKET_TYPES; i++) {
        if (context->ring_buffers[i] != NULL) {
            try_recovering_missing_packets_or_frames(context->ring_buffers[i], latency);
        }
    }

    // should_recv is only for TCP
    FATAL_ASSERT(should_recv == false);

    if (context->decrypted_packet_used) {
        LOG_ERROR("Cannot use context->decrypted_packet buffer! Still being used somewhere else!");
        return;
    }

    // Wait to receive packet over TCP, until timing out
    UDPNetworkPacket udp_network_packet;
    int recv_len = recv_no_intr(context->socket, (char*)&udp_network_packet, sizeof(udp_network_packet), 0);

    // If the packet was successfully received, then decrypt it
    if (recv_len > 0) {
        int decrypted_len;

        // Verify the reported packet length
        // This is before the `decrypt_packet` call, so the packet might be malicious
        // ~ We check recv_len against UDPNETWORKPACKET_HEADER_SIZE first, to ensure that 
        //  the access to udp_network_packet.{payload_size/aes_metadata} is in-bounds
        // ~ We check bounds on udp_network_packet.payload_size, so that the
        //  the addition check on payload_size doesn't maliciously overflow
        // ~ We make an addition check, to ensure that the payload_size matches recv_len
        if (recv_len < UDPNETWORKPACKET_HEADER_SIZE ||
            udp_network_packet.payload_size < 0 || sizeof(udp_network_packet.payload) < udp_network_packet.payload_size ||
            UDPNETWORKPACKET_HEADER_SIZE + udp_network_packet.payload_size != recv_len) {
            LOG_WARNING("The UDPPacket's payload size %d doesn't agree with recv_len %d!",
                        udp_network_packet.payload_size, recv_len);
            return;
        }

        UDPPacket udp_packet;

        if (ENCRYPTING_PACKETS) {
            // Decrypt the packet
            decrypted_len =
                decrypt_packet(&udp_packet, sizeof(udp_packet),
                               udp_network_packet.aes_metadata, udp_network_packet.payload, udp_network_packet.payload_size,
                               context->binary_aes_private_key);
            // If there was an issue decrypting it, warn and return NULL
            if (decrypted_len < 0) {
                // This is warning, since it could just be someone else sending packets,
                // Not necessarily our fault
                LOG_WARNING("Failed to decrypt packet");
                return;
            }
            // AFTER THIS LINE,
            // The contents of udp_packet are confirmed to be from the server,
            // And thus can be trusted as not maliciously formed.
        } else {
            // The decrypted packet is just in the payload, during no-encryption mode
            decrypted_len = udp_network_packet.payload_size;
            memcpy(&udp_packet, udp_network_packet.payload, udp_network_packet.payload_size);
        }
#if LOG_NETWORKING
        LOG_INFO("Received a WhistPacket of size %d over UDP", decrypted_len);
#endif

        // Verify the UDP Packet's size
        FATAL_ASSERT(decrypted_len == get_udp_packet_size(&udp_packet));

        // if the packet is a whist_segment, put the packet in a ringbuffer. Otherwise, pass it to udp_handle_message
        if (udp_packet.type == UDP_WHIST_SEGMENT) {
            WhistPacketType packet_type = udp_packet.whist_segment_data.whist_type;

            // put the decrypted packet in the correct ringbuffer
            if (context->ring_buffers[packet_type] != NULL) {
                receive_packet(context->ring_buffers[packet_type], &udp_packet);
            } else {
                // if there is no ring buffer (packet is message), store it in the 1-packet buffer instead
                // first, free the old packet there
                WhistPacket* whist_packet = (WhistPacket*)udp_packet.udp_whist_segment_data;
                FATAL_ASSERT()
                    if (context->last_packets[packet_type] != NULL) {
                        udp_free_packet(raw_context, context->last_packets[packet_type]);
                    }
                context->last_packets[packet_type] = *whist_packet;
            }
            return;
        } else {
            udp_handle_message(context, &udp_packet);
            // TODO: possibly just return udp_handle_message?
            return;
        }
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
        return;
    }
}

void* udp_get_packet(void* raw_context, WhistPacketType type) {
    UDPContext* context = raw_context;
    RingBuffer* ring_buffer = context->ring_buffers[type];

    switch (type) {
        case PACKET_MESSAGE: {
            return context->last_packets[PACKET_MESSAGE];
        }
        case PACKET_AUDIO: {
            // first, catch up audio if we're behind
            if ((ring_buffer->last_rendered_id == -1 && ring_buffer->max_id > 0) || (ring_buffer->last_rendered_id != -1 && ring_buffer->max_id - ring_buffer->last_rendered_id > MAX_NUM_AUDIO_FRAMES)) {
                ring_buffer->last_rendered_id = ring_buffer->max_id - 1;
            }
            break;
        }
        case PACKET_VIDEO: {
            // skip to the most recent iframe if necessary
            for (int i = ring_buffer->max_id; i > max(0, ring_buffer->last_rendered_id); i--) {
                if (is_ready_to_render(ring_buffer, i)) {
                    FrameData* frame_data = get_frame_at_id(ring_buffer, i);
                    VideoFrame* frame = (VideoFrame*)frame_data->frame_buffer;
                    if (frame->is_iframe) {
                        reset_stream(ring_buffer, frame_data->id);
                        break;
                    }
                }
                // possibly skip to the next iframe
                break;
            }
        }
        default: {
            LOG_ERROR("Received a packet that was of unknown type %d!", type);
        }
    }

    // then, set_rendering the next frame and give it to the decoder
    int next_to_play_id = ring_buffer->last_rendered_id + 1;
    if (is_ready_to_render(ring_buffer, next_to_play_id)) {
        return set_rendering(ring_buffer, next_to_play_id);
    } else {
        // no frame ready yet
        return NULL;
    }
}

void udp_free_packet(void* raw_context, WhistPacket* udp_packet) {
    UDPContext* context = raw_context;

    if (!context->decrypted_packet_used) {
        LOG_ERROR("Called udp_free_packet, but there was no udp_packet to free!");
        return;
    }
    if (udp_packet != &context->decrypted_packet) {
        LOG_ERROR("The wrong pointer was passed into udp_free_packet!");
    }

    // Free the one buffer
    context->decrypted_packet_used = false;
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int udp_send_udp_packet(UDPContext* context, UDPPacket* udp_packet, size_t udp_packet_size) {
    /*
     * Send a WhistPacket over UDP.
     * TODO: rename this function and emphasize that it handles networking only, any packet encryption, etc. is handled by udp_send_packet.
     */

    FATAL_ASSERT(context != NULL);
    FATAL_ASSERT(segment_size <= sizeof(UDPPacket));
    FATAL_ASSERT(udp_packet_size == get_udp_packet_size(udp_packet));

    UDPNetworkPacket udp_network_packet;
    void* data_to_send = NULL;
    if (ENCRYPTING_PACKETS) {
        // Encrypt the packet during normal operation
        int encrypted_len =
            (int)encrypt_packet(udp_network_packet.payload, &udp_network_packet.aes_metadata, udp_packet,
                                (int)udp_packet_size, context->binary_aes_private_key);
        udp_network_packet.payload_size = encrypted_len;
    } else {
        // Or, just memcpy the segment if ENCRYPTING_PACKETS is disabled
        memcpy(udp_network_packet.payload, udp_packet, udp_packet_size);
        udp_network_packet.payload_size = (int)udp_packet_size;
    }

    // The size of the udp packet that actually needs to be sent over the network
    int udp_packet_network_size =
        UDPNETWORKPACKET_HEADER_SIZE + udp_network_packet.payload_size;

    // NOTE: This doesn't interfere with clientside hotpath,
    // since the throttler only throttles the serverside
    network_throttler_wait_byte_allocation(context->network_throttler,
                                           (size_t)udp_packet_network_size);

    // If sending fails because of no buffer space available on the system, retry a few times.
    for (int i = 0; i < RETRIES_ON_BUFFER_FULL; i++) {
        whist_lock_mutex(context->mutex);
        int ret;
#if LOG_NETWORKING
        LOG_INFO("Sending a WhistPacket of size %d over UDP", (int)packet_size);
#endif
        // Send the UDPPacket over the network
        ret = send(context->socket, (const char*)&udp_network_packet, udp_packet_network_size, 0);
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

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int udp_send_packet(void* raw_context, WhistPacketType packet_type, void* payload, int payload_size,
                    int packet_id) {
    /*
     * Send data of type packet_type and ID packet_id over UDP. This breaks up the data into WhistPackets (TODO: WhistUDPPackets) small enough to be sent over UDP, applies any other transforms (e.g. encryption, error correction), and calls udp_send_udp_packet on the transformed WhistPackets (TODO: WhistUDPPackets).
     * It also updates a nack buffer to handle client nacks for lost data if necessary.
     * TODO: wrap each step of the transform (splitting, encryption, FEC) into separate functions
     * TODO: possibly make `udp_send_udp_packet` not necessarily send data over the network
     */
    UDPContext* context = raw_context;
    if (context == NULL) {
        LOG_ERROR("UDPContext is NULL");
        return -1;
    }

    // Get the nack_buffer, if there is one for this type of packet
    WhistPacket* nack_buffer = NULL;

    int type_index = (int)packet_type;
    if (type_index >= NUM_PACKET_TYPES) {
        LOG_ERROR("Type is out of bounds! Something wrong happened");
        return -1;
    }
    if (context->nack_buffers[type_index] != NULL) {
        // Sending payloads that must be split into multiple packets,
        // is only allowed for WhistPacketType's that have a nack buffer
        // This includes allowing the application of fec_ratio at all
        nack_buffer =
            context->nack_buffers[type_index][packet_id % context->nack_num_buffers[type_index]];
    }

    // Calculate number of packets needed to send the payload, rounding up.
    int num_indices = payload_size == 0 ? 1
                                        : (int)(payload_size / MAX_PAYLOAD_SIZE +
                                                (payload_size % MAX_PAYLOAD_SIZE == 0 ? 0 : 1));

    int num_fec_packets = 0;
    double fec_packet_ratio = context->fec_packet_ratios[packet_type];
    if (nack_buffer && fec_packet_ratio > 0.0) {
        num_fec_packets = get_num_fec_packets(num_indices, fec_packet_ratio);
    }

    int num_total_packets = num_indices + num_fec_packets;

// Should be something much larger than it needs to be
#define MAX_TOTAL_PACKETS 4096
    char* buffers[MAX_TOTAL_PACKETS];
    int buffer_sizes[MAX_TOTAL_PACKETS];
    if (num_total_packets > MAX_TOTAL_PACKETS) {
        LOG_FATAL("MAX_TOTAL_PACKETS is too small! Double it!");
    }

    // If nack buffer can't hold a packet with that many indices,
    // OR the original buffer is illegally large
    // OR there's no nack buffer but it's a packet that needed to be split up,
    // THEN there's a problem and we LOG_ERROR
    if ((nack_buffer && num_total_packets > context->nack_buffer_max_indices[type_index]) ||
        (nack_buffer && payload_size > context->nack_buffer_max_payload_size[type_index]) ||
        (!nack_buffer && num_total_packets > 1)) {
        LOG_ERROR("Packet is too large to send the payload! %d/%d", num_indices, num_total_packets);
        return -1;
    }

    FECEncoder* fec_encoder = NULL;
    if (num_fec_packets > 0) {
        fec_encoder = create_fec_encoder(num_indices, num_fec_packets, MAX_PAYLOAD_SIZE);
        // Pass the buffer that we'll be encoding with FEC
        fec_encoder_register_buffer(fec_encoder, (char*)payload, payload_size);
        // If using FEC, populate the buffers with FEC's buffers
        fec_get_encoded_buffers(fec_encoder, (void**)buffers, buffer_sizes);
    } else {
        int current_position = 0;
        for (int packet_index = 0; packet_index < num_indices; packet_index++) {
            // Populate the buffers directly when not using FEC
            int packet_payload_size = (int)min(payload_size - current_position, MAX_PAYLOAD_SIZE);
            buffers[packet_index] = (char*)payload + current_position;
            buffer_sizes[packet_index] = packet_payload_size;
            // Progress the pointer by this payload's size
            current_position += packet_payload_size;
        }
    }

    // Write all the packets into the packet buffer and send them all
    for (int packet_index = 0; packet_index < num_total_packets; packet_index++) {
        if (nack_buffer) {
            // Lock on a per-loop basis to not starve nack() calls
            whist_lock_mutex(context->nack_mutex[type_index]);
        }

        // Construct the packet, potentially into the nack buffer
        UDPPacket local_packet;
        UDPPacket* packet = nack_buffer ? &nack_buffer[packet_index] : &local_packet;
        packet->type = UDP_WHIST_SEGMENT;
        packet->udp_whist_segment_data.whist_type = packet_type;
        packet->udp_whist_segment_data.id = packet_id;
        packet->udp_whist_segment_data.index = (unsigned short)packet_index;
        packet->udp_whist_segment_data.num_indices = (unsigned short)num_total_packets;
        packet->udp_whist_segment_data.num_fec_indices = (unsigned short)num_fec_packets;
        packet->udp_whist_segment_data.is_a_nack = false;
        packet->udp_whist_segment_data.segment_size = buffer_sizes[packet_index];

        FATAL_ASSERT(packet->udp_whist_segment_data.segment_size <= sizeof(packet->udp_whist_segment_data.segment_data));
        memcpy(packet->udp_whist_segment_data.segment_data, buffers[packet_index], buffer_sizes[packet_index]);

        // Send the packet,
        // We don't need to propagate the return code if a subset of them drop,
        // The client will just have to nack
        udp_send_udp_packet(context, packet, get_udp_packet_size(packet));

        if (nack_buffer) {
            whist_unlock_mutex(context->nack_mutex[type_index]);
        }
    }

    if (fec_encoder) {
        destroy_fec_encoder(fec_encoder);
    }

    return 0;
}

void udp_update_network_settings(UDPContext* context, NetworkSettings network_settings) {
    int burst_bitrate = network_settings.burst_bitrate;
    double video_fec_ratio = network_settings.video_fec_ratio;
    double audio_fec_ratio = network_settings.audio_fec_ratio;

    // Set burst bitrate, if possible
    if (context->network_throttler == NULL) {
        LOG_ERROR("Tried to set the burst bitrate, but there's no network throttler!");
    } else {
        network_throttler_set_burst_bitrate(context->network_throttler, burst_bitrate);
    }

    // Set fec packet ratio
    FATAL_ASSERT(0.0 <= video_fec_ratio && video_fec_ratio <= MAX_FEC_RATIO);
    FATAL_ASSERT(0.0 <= audio_fec_ratio && audio_fec_ratio <= MAX_FEC_RATIO);
    context->fec_packet_ratios[PACKET_VIDEO] = video_fec_ratio;
    context->fec_packet_ratios[PACKET_AUDIO] = audio_fec_ratio;
}

void udp_register_ring_buffer(SocketContext* socket_context, WhistPacketType type, int max_frame_size, int num_buffers, NackPacketFn nack_packet, StreamResetFn request_stream_reset) {
    UDPContext* context = socket_context->context;

    int type_index = (int)type;
    if (type_index >= NUM_PACKET_TYPES) {
        LOG_ERROR("Type is out of bounds! Something wrong happened");
        return;
    }
    if (context->ring_buffers[type_index] != NULL) {
        LOG_ERROR("Ring Buffer has already been initialized!");
        return;
    }

    context->ring_buffers[type_index] = init_ring_buffer(type, max_frame_size, num_buffers, udp_nack_packet, udp_request_stream_reset);
}

void udp_register_nack_buffer(SocketContext* socket_context, WhistPacketType type,
                              int max_payload_size, int num_buffers) {
    /*
     * Create a nack buffer for the specified packet type which will resend packets to the client in case of data loss.
     */
    UDPContext* context = socket_context->context;

    int type_index = (int)type;
    if (type_index >= NUM_PACKET_TYPES) {
        LOG_ERROR("Type is out of bounds! Something wrong happened");
        return;
    }
    if (context->nack_buffers[type_index] != NULL) {
        LOG_ERROR("Nack Buffer has already been initialized!");
        return;
    }

    int max_num_ids = max_payload_size / MAX_PAYLOAD_SIZE + 1;
    // get max FEC ids possible, based on MAX_FEC_RATIO
    int max_fec_ids = get_num_fec_packets(max_num_ids, MAX_FEC_RATIO);

    // Adjust max ids for the maximum number of fec ids
    max_num_ids += max_fec_ids;

    // Allocate buffers than can handle the above maximum sizes
    // Memory isn't an issue here, because we'll use our region allocator,
    // so unused memory never gets allocated by the kernel
    context->nack_buffers[type_index] = malloc(sizeof(WhistPacket*) * num_buffers);
    context->nack_mutex[type_index] = whist_create_mutex();
    context->nack_num_buffers[type_index] = num_buffers;
    // This is just used to sanitize the pre-FEC buffer that's passed into send_packet
    context->nack_buffer_max_payload_size[type_index] = max_payload_size;
    context->nack_buffer_max_indices[type_index] = max_num_ids;

    // Allocate each nack buffer, based on num_buffers
    for (int i = 0; i < num_buffers; i++) {
        // Allocate a buffer of max_num_ids WhistPacket's
        context->nack_buffers[type_index][i] = allocate_region(sizeof(WhistPacket) * max_num_ids);
        // Set just the ID, but don't memset the entire region to 0,
        // Or you'll make the kernel allocate all of the memory
        for (int j = 0; j < max_num_ids; j++) {
            context->nack_buffers[type_index][i][j].id = 0;
        }
    }
}

int udp_handle_nack(UDPContext* context, WhistPacketType type, int packet_id, int packet_index) {
    /*
     * Respond to a client nack by sending the requested packet from the nack buffer if possible.
     */
    // Sanity check the request metadata
    int type_index = (int)type;
    if (type_index >= NUM_PACKET_TYPES) {
        LOG_ERROR("Type is out of bounds! Something wrong happened");
        return -1;
    }
    if (context->nack_buffers[type_index] == NULL) {
        LOG_ERROR("Nack Buffer has not been initialized!");
        return -1;
    }
    if (packet_index >= context->nack_buffer_max_indices[type_index]) {
        LOG_ERROR("Nacked Index %d is >= num indices %d!", packet_index,
                  context->nack_buffer_max_indices[type_index]);
        return -1;
    }

    // retrieve the WhistPacket from the nack buffer and send using `udp_send_udp_packet`
    // TODO: change to WhistUDPPacket
    whist_lock_mutex(context->nack_mutex[type_index]);
    WhistPacket* packet =
        &context->nack_buffers[type_index][packet_id % context->nack_num_buffers[type_index]]
                              [packet_index];

    int ret;
    if (packet->id == packet_id) {
        int len = get_packet_size(packet);
        packet->is_a_nack = true;
        // We will NACK audio all the time(See NUM_PREVIOUS_FRAMES_RESEND in audio.c). Hence logging
        // this only for video.
        if (type == PACKET_VIDEO) {
            LOG_INFO("NACKed video packet ID %d Index %d found of length %d. Relaying!", packet_id,
                     packet_index, len);
        }
        ret = udp_send_udp_packet(context, packet, len);
    } else {
        LOG_WARNING(
            "NACKed %s packet %d %d not found, ID %d was "
            "located instead.",
            type == PACKET_VIDEO ? "video" : "audio", packet_id, packet_index, packet->id);
        ret = -1;
    }

    whist_unlock_mutex(context->nack_mutex[type_index]);
    return ret;
}

void udp_destroy_socket_context(void* raw_context) {
    UDPContext* context = raw_context;

    if (context->decrypted_packet_used) {
        LOG_ERROR("Destroyed the socket context, but didn't free the most recent UDP packet!");
    }

    // Deallocate the nack buffers
    for (int type_id = 0; type_id < NUM_PACKET_TYPES; type_id++) {
        if (context->nack_buffers[type_id] != NULL) {
            for (int i = 0; i < context->nack_num_buffers[type_id]; i++) {
                deallocate_region(context->nack_buffers[type_id][i]);
            }
            free(context->nack_buffers[type_id]);
            context->nack_buffers[type_id] = NULL;
        }
    }

    closesocket(context->socket);
    if (context->network_throttler != NULL) {
        network_throttler_destroy(context->network_throttler);
    }
    whist_destroy_mutex(context->mutex);
    free(context);
}

/*
============================
Private Function Implementations
============================
*/

int create_udp_server_context(void* raw_context, int port, int recvfrom_timeout_ms,
                              int stun_timeout_ms) {
    UDPContext* context = raw_context;

    socklen_t slen = sizeof(context->addr);
    int recv_size;
    if ((recv_size = recvfrom_no_intr(context->socket, NULL, 0, 0,
                                      (struct sockaddr*)(&context->addr), &slen)) != 0) {
        LOG_WARNING("Failed to receive ack! %d %d", recv_size, get_last_network_error());
        closesocket(context->socket);
        return -1;
    }

    if (connect(context->socket, (struct sockaddr*)&context->addr, sizeof(context->addr)) == -1) {
        LOG_WARNING("Failed to connect()!");
        closesocket(context->socket);
        return -1;
    }

    if (!handshake_private_key(context)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr),
             ntohs(context->addr.sin_port));

    set_timeout(context->socket, recvfrom_timeout_ms);

    return 0;
}

int create_udp_client_context(UDPContext* context, char* destination, int port,
                              int recvfrom_timeout_ms, int stun_timeout_ms) {
    // Create UDP socket
    if ((context->socket = socketp_udp()) == INVALID_SOCKET) {
        return -1;
    }

    set_timeout(context->socket, stun_timeout_ms);

    // Client connection protocol
    context->addr.sin_family = AF_INET;
    context->addr.sin_addr.s_addr = inet_addr(destination);
    context->addr.sin_port = htons((unsigned short)port);
    if (connect(context->socket, (struct sockaddr*)&context->addr, sizeof(context->addr)) == -1) {
        LOG_WARNING("Failed to connect()!");
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Connecting to server...");

    // Send Ack
    if (udp_ack(context) < 0) {
        LOG_WARNING("Could not send ack to server %d\n", get_last_network_error());
        closesocket(context->socket);
        return -1;
    }

    if (!handshake_private_key(context)) {
        LOG_WARNING("Could not complete handshake!");
        closesocket(context->socket);
        return -1;
    }

    LOG_INFO("Connected to server on %s:%d! (Private %d)\n", inet_ntoa(context->addr.sin_addr),
             port, ntohs(context->addr.sin_port));

    set_timeout(context->socket, recvfrom_timeout_ms);

    return 0;
}

// TODO: move stream reset function here too
//
void nack_packet(SocketContext* socket_context, WhistPacketType frame_type, int id, int index) {
    /*
        Nack the packet at ID id and index index.

        Arguments:
            frame_type (WhistPacketType): the packet type
            id (int): Frame ID of the packet
            index (int): index of the packet
    */
    WhistClientMessage wcmsg = {0};
    wcmsg.type = MESSAGE_NACK;
    wcmsg.simple_nack.type = frame_type;
    wcmsg.simple_nack.id = id;
    wcmsg.simple_nack.index = index;
    // TODO: send this over the given socket_context using udp_send_packet
    send_wcmsg(&wcmsg);
}

/*
============================
Public Function Implementations
============================
*/

bool create_udp_socket_context(SocketContext* network_context, char* destination, int port,
                               int recvfrom_timeout_ms, int connection_timeout_ms, bool using_stun,
                               char* binary_aes_private_key) {
    /*
        Create a UDP socket context

        Arguments:
            context (SocketContext*): pointer to the SocketContext struct to initialize
            destination (char*): the destination address, NULL means act as a server
            port (int): the port to bind over
            recvfrom_timeout_ms (int): timeout, in milliseconds, for recvfrom
            connection_timeout_ms (int): timeout, in milliseconds, for socket connection
            using_stun (bool): Whether or not to use STUN
            binary_aes_private_key (char*): The 16byte AES key to use

        Returns:
            (int): -1 on failure, 0 on success
    */

    // Populate function pointer table
    network_context->get_packet = udp_get_packet;
    network_context->update = udp_update;
    network_context->free_packet = udp_free_packet;
    network_context->send_packet = udp_send_packet;
    network_context->destroy_socket_context = udp_destroy_socket_context;

    // Create the UDPContext, and set to zero
    UDPContext* context = safe_malloc(sizeof(UDPContext));
    memset(context, 0, sizeof(UDPContext));
    network_context->context = context;

    // if dest is NULL, it means the context will be listening for income connections
    if (destination == NULL) {
        if (network_context->listen_socket == NULL) {
            LOG_ERROR("listen_socket not provided");
            return false;
        }
        /*
            for udp, transfer the ownership to UDPContext.
            when UDPContext is destoryed, the transferred listen_socket should be closed.
        */
        context->socket = *network_context->listen_socket;
        *network_context->listen_socket = INVALID_SOCKET;
    }

    // Map Port
    if ((int)((unsigned short)port) != port) {
        LOG_ERROR("Port invalid: %d", port);
    }
    port = port_mappings[port];

    context->timeout = recvfrom_timeout_ms;
    context->mutex = whist_create_mutex();
    memcpy(context->binary_aes_private_key, binary_aes_private_key,
           sizeof(context->binary_aes_private_key));

    if (destination == NULL) {
        // On the server, we create a network throttler to limit the
        // outgoing bitrate.
        context->network_throttler =
            network_throttler_create(UDP_NETWORK_THROTTLER_BUCKET_MS, false);
    } else {
        context->network_throttler = NULL;
    }

    for (int i = 0; i < NUM_PACKET_TYPES; i++) {
        context->fec_packet_ratios[i] = 0.0;
    }

    int ret;
    if (using_stun) {
        if (destination == NULL)
            ret = create_udp_server_context_stun(context, port, recvfrom_timeout_ms,
                                                 connection_timeout_ms);
        else
            ret = create_udp_client_context_stun(context, destination, port, recvfrom_timeout_ms,
                                                 connection_timeout_ms);
    } else {
        if (destination == NULL)
            ret = create_udp_server_context(context, port, recvfrom_timeout_ms,
                                            connection_timeout_ms);
        else
            ret = create_udp_client_context(context, destination, port, recvfrom_timeout_ms,
                                            connection_timeout_ms);
    }

    if (ret == 0) {
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

/*
============================
Private Function Implementation
============================
*/

#include <stddef.h>
int get_udp_packet_size(UDPPacket* udp_packet) {
    switch(udp_packet->type) {
    UDP_WHIST_SEGMENT: {
        return offsetof(UDPPacket, udp_whist_segment_data.segment_data) + udp_packet->udp_whist_segment_data.segment_size;
    }
    UDP_NACK:
        return offsetof(UDPPacket, udp_nack_data) + sizeof(udp_packet->udp_nack_data);
    UDP_BITARRAY_NACK:
        return offsetof(UDPPacket, udp_bitarray_nack_data) + sizeof(udp_packet->udp_bitarray_nack_data);
    UDP_STREAM_RESET:
        return offsetof(UDPPacket, udp_stream_reset_data) + sizeof(udp_packet->udp_stream_reset_data);
    UDP_PING:
        return offsetof(UDPPacket, udp_ping_data) + sizeof(udp_packet->udp_ping_data);
    UDP_NETWORK_SETTINGS:
        return offsetof(UDPPacket, udp_network_settings_data) + sizeof(udp_packet->udp_network_settings_data);
    default:
        LOG_FATAL("Unknown UDP Packet Type: %d", udp_packet->type);
    }
}

int udp_ack(UDPContext* context) {
    return send(context->socket, NULL, 0, 0);
};

int udp_handle_message(UDPContext* context, UDPPacket* packet) {
    switch (packet->type) {
        case UDP_NACK: {
                           // we received a nack from the client, respond
                           return udp_handle_nack(context, packet->udp_nack_data.whist_type, packet->udp_nack_data.id, packet->udp_nack_data.index);
                       }
        case UDP_BITARRAY_NACK: {
                                    // nack for everything
                                    BitArray *bit_arr = bit_array_create(packet->udp_bitarray_nack_data.numBits);
                                    bit_array_clear_all(bit_arr);

                                    memcpy(bit_array_get_bits(bit_arr), packet->udp_bitarray_nack_data.ba_raw,
                                            BITS_TO_CHARS(packet->udp_bitarray_nack_data.numBits));

                                    for (int i = packet->udp_bitarray_nack_data.index; i < packet->udp_bitarray_nack_data.numBits; i++) {
                                        if (bit_array_test_bit(bit_arr, i)) {
                                            udp_handle_nack(context, packet->udp_bitarray_nack_data.type, packet->udp_bitarray_nack_data.id,
                                                    i);
                                        }
                                    }
                                    bit_array_free(bit_arr);
                                    return 0;
                                }
        case UDP_STREAM_RESET: {
                                   // currently unimplemented
                                   return 0;
                               }
        case UDP_PING: {
                           // currently unimplemented
                           return 0;
                       }
        case UDP_NETWORK_SETTINGS: {
                                       udp_update_network_settings(context, packet->udp_network_settings_data.network_settings);
                                       return 0;
                                   }
        default: {
                     LOG_ERROR("Packet of unknown type!");
                 }
    }
}

StreamResetData udp_get_pending_stream_reset_request(SocketContext* socket_context, WhistPacketType type) {
    UDPContext* context = (UDPContext*)socket_context->context;
    StreamResetData data = {0};
    data.pending_stream_reset = reset_data[type].pending_stream_reset;
    data.greatest_failed_id = reset_data[type].greatest_failed_id;
    if (reset_data[type].pending_stream_reset) {
        // server has receeived the stream reset request
        reset_data[type].pending_stream_reset = false;
    }
    return data;
}

void udp_handle_stream_reset(UDPContext* context, WhistPacketType type, int greatest_failed_id) {
    context->reset_data[type].pending_stream_reset = true;
    context->reset_data[type].greatest_failed_id = max(greatest_failed_id, context->reset_data[type].greatest_failed_id);
}

int udp_nack_packet(SocketContext* socket_context, WhistPacketType type, int id, int index) {
    UDPContext* context = (UDPContext*)socket_context->context;
    // Nack for the given packet by sending a UDPPacket of type UDP_NACK
    UDPPacket packet = {0};
    packet.type = UDP_NACK;
    packet.udp_nack_data.whist_type = type;
    packet.id = id;
    packet.index = index;
    return udp_send_udp_packet(context, &packet, get_udp_packet_size(&packet));
}

int udp_request_stream_reset(SocketContext* socket_context, WhistPacketType type, int greatest_failed_id) {
    UDPContext* context = (UDPContext*)socket_context->context;
    // Tell the server the client fell too far behind
    UDPPacket packet = {0};
    packet.type = UDP_STREAM_RESET;
    packet.udp_stream_reset_data.whist_type = type;
    packet.udp_stream_reset_data.greatest_failed_id = greatest_failed_id;
    return udp_send_udp_packet(context, &packet, get_udp_packet_size(&packet));
}
