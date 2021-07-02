#include "avpacket_buffer.h"

void write_packets_to_buffer(int num_packets, AVPacket* packets, int* buf) {
    /*
        Store the first num_packets AVPackets contained in packets into buf. buf will contain
        the following data: (number of packets)(size of each packet)(data of each packet).

        Arguments:
            num_packets (int): the number of packets to store in buf
            packets (AVPacket*): array of packets to read packets from
            buf (int*): memory buffer for storing the packets
    */
    *buf = num_packets;
    buf++;
    for (int i = 0; i < num_packets; i++) {
        *buf = packets[i].size;
        buf++;
    }
    char* char_buf = (void*)buf;
    for (int i = 0; i < num_packets; i++) {
        memcpy(char_buf, packets[i].data, packets[i].size);
        char_buf += packets[i].size;
    }
}

int extract_packets_from_buffer(void* buffer, int buffer_size, AVPacket* packets) {
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
    if (buffer_size < 4) {
        LOG_FATAL("Received a buffer size of %d, too small!", buffer_size);
    }

    // first entry: number of packets
    int* int_buffer = buffer;
    int num_packets = *int_buffer;
    int_buffer++;

    // we compute the size of metadata + packets and ensure it agrees with buffer_size
    int computed_size = sizeof(int);

    // find buffer sizes
    for (int i = 0; i < num_packets; i++) {
        // make sure to clear the packet data
        av_packet_unref(&packets[i]);
        // next entry in buffer is packet size
        packets[i].size = *int_buffer;
        computed_size += sizeof(int) + packets[i].size;
        int_buffer++;
    }

    if (buffer_size != computed_size) {
        LOG_ERROR(
            "Given Buffer Size did not match computed buffer size: given %d vs "
            "computed %d",
            buffer_size, computed_size);
        return -1;
    }

    // the rest of the buffer is each packet's data
    char* char_buffer = (void*)int_buffer;
    for (int i = 0; i < num_packets; i++) {
        // set packet data
        packets[i].data = (void*)char_buffer;
        char_buffer += packets[i].size;
    }

    // return number of packets
    return num_packets;
}
