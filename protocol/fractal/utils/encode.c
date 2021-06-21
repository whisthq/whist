#include "encode.h"

void write_packets_to_buffer(int num_packets, AVPacket *packets, int *buf) {
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
    char *char_buf = (void *)buf;
    for (int i = 0; i < num_packets; i++) {
        memcpy(char_buf, packets[i].data, packets[i].size);
        char_buf += packets[i].size;
    }
}
