#ifndef DECODE_H
#define DECODE_H

AVPacket** extract_packets(void* buffer, int buffer_size);

void free_packets(AVPacket** packets, int num_packets);
