/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file error_codes.h
 * @brief Error code utility functions.
 */

#include "whist.h"
#include "error_codes.h"

const char *whist_error_string(WhistStatus error_code) {
    static const char *const error_strings[] = {
        [-WHIST_SUCCESS] = "success",
        [-WHIST_ERROR_UNKNOWN] = "unknown error",
        [-WHIST_ERROR_INVALID_ARGUMENT] = "invalid argument",
        [-WHIST_ERROR_NOT_FOUND] = "not found",
        [-WHIST_ERROR_OUT_OF_MEMORY] = "out of memory",
        [-WHIST_ERROR_SYNTAX] = "syntax error",
        [-WHIST_ERROR_OUT_OF_RANGE] = "out of range",
        [-WHIST_ERROR_TOO_LONG] = "too long",
        [-WHIST_ERROR_ALREADY_SET] = "already set",
        [-WHIST_ERROR_IO] = "I/O error",
        [-WHIST_ERROR_WOULD_BLOCK] = "operation would block",
        [-WHIST_ERROR_IN_PROGRESS] = "operation in progress",
        [-WHIST_ERROR_NOT_CONNECTED] = "socket not connected",
        [-WHIST_ERROR_ADDRESS_IN_USE] = "address in use",
        [-WHIST_ERROR_ADDRESS_NOT_AVAILABLE] = "address not available",
        [-WHIST_ERROR_ADDRESS_UNREACHABLE] = "address unreachable",
        [-WHIST_ERROR_CONNECTION_RESET] = "connection reset",
        [-WHIST_ERROR_CONNECTION_REFUSED] = "connection refused",
        [-WHIST_ERROR_NO_BUFFERS] = "no buffers",
    };
    if (error_code <= 0 && (size_t)-error_code < ARRAY_LENGTH(error_strings) &&
        error_strings[-error_code]) {
        return error_strings[-error_code];
    } else {
        return "invalid error code";
    }
}
