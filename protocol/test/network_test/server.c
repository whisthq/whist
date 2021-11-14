#include "utils.h"

int main(int argc, char* argv[]) {
    fractal_init_multithreading();
    init_logger();
    init_statistic_logger();

    SocketContext udp_socket;
    while(!create_udp_socket_context(&udp_socket, NULL, NETWORK_TEST_PORT, 1, CONNECTION_TIMEOUT, false, DEFAULT_BINARY_PRIVATE_KEY)) {
        LOG_INFO("Waiting for connection from client...");
    }

    clock start_stream_timer;
    start_timer(&start_stream_timer);

    clock last_frame_timer;
    start_timer(&last_frame_timer);

    char buf[MAX_PAYLOAD_SIZE];

    int frame_size = STARTING_BITRATE / 8.0 / FPS;

    char* buf = safe_malloc(frame_size);
    memset(buf, 0, frame_size);

    int frame_id = 1;
    while(1) {
        if (get_timer(last_frame_timer) > 1.0/FPS * 1000) {
            // Send a frame
            send_packet(&udp_socket, PACKET_VIDEO, buf, frame_size, frame_id);
            frame_id++;
        }
    }

    destroy_socket_context(&udp_socket);
    destroy_logger();
}