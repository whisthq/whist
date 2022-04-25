#include "common.h"
#include "virtual.h"

#define VIRTUAL_FUNCTION_TABLE_BINDING(return_type, name, ...) virtual_##name,
static const WhistFrontendFunctionTable virtual_function_table = {
    FRONTEND_API(VIRTUAL_FUNCTION_TABLE_BINDING)};

const WhistFrontendFunctionTable* virtual_get_function_table(void) {
    return &virtual_function_table;
}
