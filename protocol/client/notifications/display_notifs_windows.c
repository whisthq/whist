#include "notifications.h"

#include <whist/logging/logging.h>

int native_show_notification(WhistPacket *packet) {
    LOG_WARNING("Notification display not implemented on windows");
    return -1;
}