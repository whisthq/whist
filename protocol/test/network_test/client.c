#include "utils.h"
#include <fractal/network/ringbuffer.h>

int main(int argc, char* argv[]) {
    fractal_init_multithreading();
    init_logger();
    init_statistic_logger();

    const char* server_ip = NULL;
    if (argc != 2) {
        LOG_FATAL("Number of arguments is wrong!");
    } else {
        server_ip = argv[1];
    }

    SocketContext udp_socket;
    create_udp_socket_context(&udp_socket, server_ip, NETWORK_TEST_PORT, 1, CONNECTION_TIMEOUT, false, DEFAULT_BINARY_PRIVATE_KEY);

    RingBuffer* ring_buffer = init_ring_buffer(PACKET_VIDEO, 100, NULL);

    while(1) {
        FractalPacket* packet = read_packet(&udp_socket, true);

        receive_packet(ring_buffer, packet);

        free_packet(packet);
    }

    destroy_socket_context(&udp_socket);
    destroy_logger();
}