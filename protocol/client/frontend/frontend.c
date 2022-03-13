#include "frontend.h"
#include <whist/core/whist.h>


static WhistFrontend* whist_frontend_create(const WhistFrontendFunctionTable* function_table) {
    WhistFrontend* frontend = malloc(sizeof(WhistFrontend));
    frontend->context = NULL;
    frontend->call = function_table;
    if (frontend->call->init(frontend) != 0) {
        free(frontend);
        return NULL;
    }
    return frontend;
}

WhistFrontend* whist_frontend_create_sdl(void) {
    return whist_frontend_create(sdl_get_function_table());
}




void whist_frontend_destroy(WhistFrontend* frontend) {
    FATAL_ASSERT(frontend != NULL);
    frontend->call->destroy(frontend);
    free(frontend);
}
