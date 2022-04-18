#include "common.h"
#include "sdl.h"

#define SDL_FUNCTION_TABLE_BINDING(return_type, name, ...) sdl_##name,
static const WhistFrontendFunctionTable sdl_function_table = {
    FRONTEND_API(SDL_FUNCTION_TABLE_BINDING)};

const WhistFrontendFunctionTable* sdl_get_function_table(void) { return &sdl_function_table; }
