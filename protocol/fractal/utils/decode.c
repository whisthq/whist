#include "decode.h"

int extract_packets_from_buffer(void* buffer, int buffer_size, AVPacket* packets) {
    /*
        Read the encoded packets stored in buffer into packets. The buffer should have been filled
       using read_packets_into_buffer.

        Arguments:
            buffer (void*): Buffer containing encoded packets. Format: (number of packets)(size of
       each packet)(data of each packet) buffer_size (int): Provided size of the buffer. This will
       be compared against the size implied by the buffer's metadata; no data will be copied if
       there is a mismatch. packets (AVPacket*): array of encoded packets. Packets will be
       unreferenced before being filled with new data.

        Returns:
            (int): 0 on success, -1 on failure
            */
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
