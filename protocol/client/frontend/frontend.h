#ifndef WHIST_CLIENT_FRONTEND_H
#define WHIST_CLIENT_FRONTEND_H

#include <whist/core/whist.h>
#include <whist/core/error_codes.h>
#include "frontend_structs.h"
#include "api.h"

// TODO: Opaquify this
#define FRONTEND_FUNCTION_TABLE_DECLARATION(return_type, name, ...) \
    return_type (*name)(__VA_ARGS__);
typedef struct WhistFrontendFunctionTable {
    FRONTEND_API(FRONTEND_FUNCTION_TABLE_DECLARATION)
} WhistFrontendFunctionTable;
#undef FRONTEND_FUNCTION_TABLE_DECLARATION

struct WhistFrontend {
    void* context;
    unsigned int id;
    const WhistFrontendFunctionTable* call;
    const char* type;
};

#define WHIST_FRONTEND_SDL "sdl"
#define WHIST_FRONTEND_EXTERNAL "external"

#define FRONTEND_HEADER_DECLARATION(return_type, name, ...) \
    return_type whist_frontend_##name(__VA_ARGS__);
FRONTEND_API(FRONTEND_HEADER_DECLARATION)
#undef FRONTEND_HEADER_DECLARATION

WhistFrontend* whist_frontend_create(const char* type);
unsigned int whist_frontend_get_id(WhistFrontend* frontend);
const char* whist_frontend_event_type_string(FrontendEventType type);

#endif  // WHIST_CLIENT_FRONTEND_H
