#ifndef SERVER_MESSAGE_HANDLER_H
#define SERVER_MESSAGE_HANDLER_H

#include "../fractal/core/fractal.h"

#include <stddef.h>

int handleServerMessage(FractalServerMessage *fmsg, size_t fmsg_size);

#endif  // SERVER_MESSAGE_HANDLER_H
