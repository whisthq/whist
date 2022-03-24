#include "avpacket_buffer.h"

int write_avpackets_to_buffer(void* raw_buffer, int buffer_size, AVPacket* packets,
                              int num_packets) {
    /*
        Store the first num_packets AVPackets contained in packets into buf.
        Buf will contain the following data: (number of packets)(sizes of each packet)(data of each
       packet)
    */

    // Peg to a value to ensure that client/server agree
    FATAL_ASSERT(AV_INPUT_BUFFER_PADDING_SIZE == 64);

    // As char*
    char* char_buffer = (void*)raw_buffer;

    // The final size of the buffer
    int result_size = 0;

    // First, the number of packets (4 bytes)
    result_size += sizeof(int);
    FATAL_ASSERT(buffer_size >= result_size);
    *(int*)(char_buffer) = num_packets;

    // Then, we copy each packet's size (4 bytes each)
    result_size += num_packets * sizeof(int);
    FATAL_ASSERT(buffer_size >= result_size);
    for (int i = 0; i < num_packets; i++) {
        *(int*)(char_buffer + (i + 1) * sizeof(int)) = packets[i].size;
    }

    // Now, we write the packet data
    for (int i = 0; i < num_packets; i++) {
        // Copy the data into the charbuf
        if (buffer_size < result_size + packets[i].size) {
            return -1;
        }
        memcpy(char_buffer + result_size, packets[i].data, packets[i].size);
        result_size += packets[i].size;
        // Give enough zero-padding so that it can be a valid avpkt->data
        if (buffer_size < result_size + AV_INPUT_BUFFER_PADDING_SIZE) {
            return -1;
        }
        memset(char_buffer + result_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
        result_size += AV_INPUT_BUFFER_PADDING_SIZE;
    }

    // Return the result size
    return result_size;
}

int extract_avpackets_from_buffer(AVPacket** packets, int max_num_packets, void* buffer,
                                  int buffer_size) {
    /*
        Read the encoded packets stored in buffer into packets.
        The buffer should have been filled using read_packets_into_buffer.
    */

    // Peg to a value to ensure that client/server agree
    FATAL_ASSERT(AV_INPUT_BUFFER_PADDING_SIZE == 64);

    FATAL_ASSERT(buffer != NULL);
    // Init all the packets to NULL
    for (int i = 0; i < max_num_packets; i++) {
        packets[i] = NULL;
    }

    // First entry: number of packets
    FATAL_ASSERT(buffer_size >= (int)sizeof(int));
    int* int_buffer = buffer;
    int num_packets = *int_buffer;
    FATAL_ASSERT(num_packets <= max_num_packets);

    // We compute the size of metadata (Later +packet sizes),
    // and ensure it agrees with the total buffer_size
    int total_size = (int)sizeof(int) + num_packets * (int)sizeof(int);
    // Ensure enough size for the metadata
    FATAL_ASSERT(buffer_size >= total_size);

    // Find buffer sizes from metadata
    int_buffer++;
    for (int i = 0; i < num_packets; i++) {
        packets[i] = av_packet_alloc();
        // Set the packet size
        packets[i]->size = *int_buffer;
        int_buffer++;
        // Track total size
        total_size += packets[i]->size + AV_INPUT_BUFFER_PADDING_SIZE;
    }

    // Confirm the buffer has the correct amount of data
    FATAL_ASSERT(buffer_size == total_size);

    // The rest of the buffer is each packet's data
    char* char_buffer = (char*)int_buffer;
    for (int i = 0; i < num_packets; i++) {
        // Set packet data
        packets[i]->data = (uint8_t*)char_buffer;
        // Track buffer location
        char_buffer += packets[i]->size + AV_INPUT_BUFFER_PADDING_SIZE;
    }

    // return number of packets
    return num_packets;
}
