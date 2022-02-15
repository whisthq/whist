#include <whist/core/whist.h>

#include "cursor.h"

size_t get_cursor_info_size(WhistCursorInfo* cursor_info) {
    FATAL_ASSERT(cursor_info != NULL);

    if (cursor_info->using_png) {
        return sizeof(WhistCursorInfo) + cursor_info->png_size;
    } else {
        return sizeof(WhistCursorInfo);
    }
}
