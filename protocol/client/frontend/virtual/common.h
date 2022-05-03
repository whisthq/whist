#ifndef WHIST_CLIENT_FRONTEND_VIRTUAL_COMMON_H
#define WHIST_CLIENT_FRONTEND_VIRTUAL_COMMON_H

#include <whist/core/whist.h>
#include "../api.h"
#include "../frontend.h"

typedef struct VirtualFrontendContext {
    int width;
    int height;
    int dpi;
} VirtualFrontendContext;

#define VIRTUAL_COMMON_HEADER_ENTRY(return_type, name, ...) return_type virtual_##name(__VA_ARGS__);
FRONTEND_API(VIRTUAL_COMMON_HEADER_ENTRY)
#undef VIRTUAL_COMMON_HEADER_ENTRY

#endif  // WHIST_CLIENT_FRONTEND_VIRTUAL_COMMON_H
