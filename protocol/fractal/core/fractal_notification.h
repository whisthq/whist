#ifndef FRACTAL_NOTIFICATION_H
#define FRACTAL_NOTIFICATION_H

// Randomly chosen, 
#define MAX_TITLE_LEN 1000
#define MAX_MESSAGE_LEN 10000

typedef struct FractalNotification {
    char title[MAX_TITLE_LEN];
    char message[MAX_MESSAGE_LEN];
} FractalNotification;



#endif //FRACTAL_NOTIFICATION_H