/**
 * @copyright Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file avpacket_buffer.c
 * @brief Serialisation/deserialisation for packet batches.
 */

#include <libavutil/intreadwrite.h>

#include "avpacket_buffer.h"

void write_avpackets_to_buffer(int num_packets, AVPacket** packets, uint8_t* buffer) {
    /*
        Store the first num_packets AVPackets contained in packets into buf. buf will contain
        the following data: (number of packets)(size of each packet)(data of each packet).

        Arguments:
            num_packets (int): the number of packets to store in buf
            packets (AVPacket*): array of packets to read packets from
            buffer (uint8_t*): memory buffer for storing the packets
    */
    if (num_packets > 10) {
        LOG_FATAL("Invalid number of packets to write to buffer: %d.", num_packets);
    }

    AV_WL32(buffer, num_packets);
    size_t pos = 4;

    for (int i = 0; i < num_packets; i++) {
        AV_WL32(buffer + pos, packets[i]->size);
        pos += 4;
    }

    for (int i = 0; i < num_packets; i++) {
        memcpy(buffer + pos, packets[i]->data, packets[i]->size);
        pos += packets[i]->size;
    }
}

int extract_avpackets_from_buffer(uint8_t* buffer, size_t buffer_size, AVPacket** packets) {
    /*
        Read the encoded packets stored in buffer into packets. The buffer should have been filled
        using read_packets_into_buffer.

        Arguments:
            buffer (void*): Buffer containing encoded packets. Format: (number of packets)(size of
                each packet)(data of each packet)

            buffer_size (int): Provided size of the buffer. This will be compared against the size
                implied by the buffer's metadata; no data will be copied if there is a mismatch.

            packets (AVPacket*): array of encoded packets. Packets will be unreferenced before being
                filled with new data.

        Returns:
            (int): 0 on success, -1 on failure
    */

    if (buffer == NULL) {
        LOG_FATAL("Received a NULL buffer!");
    }
    if (buffer_size < 4 || buffer_size > MAX_VIDEOFRAME_DATA_SIZE) {
        LOG_FATAL("Invalid packet buffer size: %zu bytes.", buffer_size);
    }

    // first entry: number of packets
    uint32_t num_packets = AV_RL32(buffer);

    // We currently expect exactly one packet in each buffer, but
    // previously larger numbers were allowed.  We accept but warn if
    // there is more than one packet.
    if (num_packets < 1 || num_packets > 10) {
        LOG_FATAL("Invalid number of packets in buffer: %" PRIu32 " packets found.", num_packets);
    }
    if (num_packets != 1) {
        LOG_ERROR("Unexpected number of packets in buffer: %" PRIu32 " packets found.",
                  num_packets);
    }

    size_t size_pos = 4;
    size_t data_pos = size_pos + 4 * num_packets;
    for (uint32_t p = 0; p < num_packets; p++) {
        // Extract the size of this packet.
        uint32_t packet_size = AV_RL32(buffer + size_pos);
        size_pos += 4;

        // Validate packet size.
        if (packet_size == 0 || data_pos + packet_size > buffer_size) {
            LOG_FATAL("Invalid packet size: %" PRIu32 " bytes at position %zu.", packet_size,
                      data_pos);
        }

        if (packets[p] == NULL) {
            // Allocate a new packet.
            packets[p] = av_packet_alloc();
            FATAL_ASSERT(packets[p]);
        } else {
            // Unreference the previous packet (the decoder may still
            // hold a reference to the data).
            av_packet_unref(packets[p]);
        }
        AVPacket* pkt = packets[p];

        // Allocate a new refcounted buffer for the packet data.
        // (This also includes the necessary zeroed padding.)
        int res = av_new_packet(pkt, packet_size);
        FATAL_ASSERT(res == 0);

        // Copy the packet data to the packet.
        memcpy(pkt->data, buffer + data_pos, packet_size);
        data_pos += packet_size;
    }

    // return number of packets
    return num_packets;
}
