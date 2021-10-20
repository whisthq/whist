#include "udp.h"

NetworkContext* create_udp_network_context(SocketContext* context, char* destination, int port,
                                           int recvfrom_timeout_s, int connection_timeout_ms,
                                           char* binary_aes_private_key) {
    // Initialize the SocketContext with using_stun as false
    if (create_udp_context(context, destination, port, recvfrom_timeout_s, connection_timeout_ms,
                           false, binary_aes_private_key) < 0) {
        LOG_WARNING("Failed to create UDP network context!");
        return NULL;
    };

    // Create the nextwork_context, set to zero, and load its functions
    NetworkContext* network_context = safe_malloc(sizeof(NetworkContext));

    // Functions common to all network contexts
    network_context->sendp = sendp;
    network_context->recvp = recvp;
    network_context->ack = ack;

    // Funcitons common to only UDP contexts
    network_context->read_packet = read_udp_packet;
    network_context->send_packet_from_payload = send_udp_packet_from_payload;
    network_context->free_packet = NULL;

    // Add in the socket context
    network_context->context = context;

    return network_context;
}
