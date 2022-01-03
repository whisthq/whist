#include "notifications.h"

int display_notification(WhistPacket *packet) {
    LOG_INFO("GOT PACKET: %s\n", packet->data);

    return 0;
}