#include "tcp_stun.h"

NetworkContext* create_tcp_stun_network_context(SocketContext* context, char* destination, int port,
                                                int recvfrom_timeout_s, int connection_timeout_ms,
                                                char* binary_aes_private_key) {
    // Initialize the SocketContext, with using_stun as true
    if (create_tcp_context(context, destination, port, recvfrom_timeout_s, connection_timeout_ms,
                           true, binary_aes_private_key) < 0) {
        LOG_WARNING("Failed to create TCP STUN network context!");
        return NULL;
    };

    // Create the nextwork_context, set to zero, and load its functions
    NetworkContext* network_context = safe_malloc(sizeof(NetworkContext));

    // Functions common to all network contexts
    network_context->sendp = sendp;
    network_context->recvp = recvp;
    network_context->ack = ack;

    // Funcitons common to only TCP contexts
    network_context->read_tcp_packet = read_tcp_packet;
    network_context->send_tcp_packet_from_payload = send_tcp_packet_from_payload;
    network_context->free_tcp_packet = free_tcp_packet;

    // Explicitly NULL out UDP functions, they shouldn't be usable
    network_context->read_udp_packet = NULL;
    network_context->send_udp_packet_from_payload = NULL;

    // Add in the socket context
    network_context->context = context;

    return network_context;
}