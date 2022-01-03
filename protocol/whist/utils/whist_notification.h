#ifndef WHIST_NOTIFICATION_H
#define WHIST_NOTIFICATION_H

// Arbitrarily chosen
#define MAX_TITLE_LEN 500
#define MAX_MESSAGE_LEN 500

typedef struct WhistNotification {
    char title[MAX_TITLE_LEN];
    char message[MAX_MESSAGE_LEN];
} WhistNotification;

#endif //WHIST_NOTIFICATION_H