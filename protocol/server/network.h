#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H

int broadcastAck(void);

int broadcastUDPPacket(FractalPacketType type, void *data, int len, int id, int burst_bitrate,
                       FractalPacket *packet_buffer, int *packet_len_buffer);

int broadcastTCPPacket(FractalPacketType type, void *data, int len);

int tryGetNextMessageTCP(int client_id, FractalClientMessage **fcmsg, size_t *fcmsg_size);

int tryGetNextMessageUDP(int client_id, FractalClientMessage *fcmsg, size_t *fcmsg_size);

int connectClient(int id);

int disconnectClient(int id);

int disconnectClients(void);

#endif  // SERVER_NETWORK_H
