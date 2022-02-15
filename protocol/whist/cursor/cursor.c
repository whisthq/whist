#include "cursor.h"

size_t get_cursor_info_size(WhistCursorInfo* cursor_info) {
    if (!cursor_info) {
        return 0;
    } else if (cursor_info->using_png) {
        return sizeof(WhistCursorInfo) + cursor_info->png_size;
    } else {
        return sizeof(WhistCursorInfo);
    }
}
