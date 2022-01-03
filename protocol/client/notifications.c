#include "notifications.h"

int display_notification(WhistPacket *packet) {
    printf("%s\n", packet->data);

    return 0;
}