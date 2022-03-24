/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file error_codes.h
 * @brief Error code definitions.
 */
#ifndef WHIST_CORE_ERROR_CODES_H
#define WHIST_CORE_ERROR_CODES_H

/**
 * @defgroup error_codes Error Codes
 *
 * Common error codes for Whist functions.
 *
 * @{
 */

/**
 * Error codes.
 *
 * Add to this enum if you have a function which can fail in a new way
 * which would be useful for the caller to be able to know about and
 * distinguish from other possible failure modes.
 */
typedef enum WhistStatus {
    /**
     * Success - not an error.
     *
     * For use when a function succeeds with no particular result.
     * Positive return values are also considered success, possibly with
     * additional meaning (such as a number of bytes processed).
     */
    WHIST_SUCCESS = 0,
    /**
     * Something unknown went wrong.
     *
     * This is normally used as an error return by functions which do
     * not return more specific error codes.  It should be avoided when
     * more specific information is available.
     */
    WHIST_ERROR_UNKNOWN = -1,
    /**
     * Invalid argument.
     *
     * Returned when the arguments to a function are invalid, or some
     * necessary precondition is not fulfilled.
     *
     * Corresponds to EINVAL or WSAEINVAL.
     */
    WHIST_ERROR_INVALID_ARGUMENT = -2,
    /**
     * Something needed was not found.
     *
     * May come from ENOENT, ESRCH, ENODEV, or similar codes.
     */
    WHIST_ERROR_NOT_FOUND = -3,
    /**
     * A needed memory allocation failed.
     *
     * This should only be returned if the situation is likely to be
     * recoverable - if it will be fatal, just assert success inside
     * the function (e.g. by using safe_malloc()).
     *
     * Corresponds to ENOMEM.
     */
    WHIST_ERROR_OUT_OF_MEMORY = -4,
    /**
     * Syntax error.
     *
     * Returned by a parsing operation which encounters data which does
     * not match the expected syntax.
     */
    WHIST_ERROR_SYNTAX = -5,
    /**
     * Out-of-range.
     *
     * Returned when a value is not within the expected range.
     */
    WHIST_ERROR_OUT_OF_RANGE = -6,
    /**
     * Too long.
     *
     * Returned when something is too long to be used.  For example, a
     * string or object which does not fit in a fixed-size buffer.
     */
    WHIST_ERROR_TOO_LONG = -7,
    /**
     * Already set.
     *
     * Returned when an operation tries to set something which is
     * already set, and overwriting does not make sense.
     */
    WHIST_ERROR_ALREADY_SET = -8,
    /**
     * I/O error.
     *
     * Some input or output operation failed.
     *
     * May come from EIO, EPIPE, or similar codes.
     */
    WHIST_ERROR_IO = -9,
    /**
     * Operation would block for longer than wanted.
     *
     * Returned for non-blocking sockets when the operation is unable to
     * complete immediately, and also when a timeout is set and elapses.
     *
     * Corresponds to EAGAIN, EWOULDBLOCK or WSAWOULDBLOCK.
     */
    WHIST_ERROR_WOULD_BLOCK = -10,
    /**
     * Operation has started but not finished.
     *
     * Returned by a connect operation.
     *
     * Corresponds to EINPROGRESS, WSAWOULDBLOCK.
     */
    WHIST_ERROR_IN_PROGRESS = -11,
    /**
     * Operation requires a connection but doesn't have one.
     *
     * Returned when a connection is required but not available.  This
     * includes packet operations like send on an unconnected socket.
     *
     * Corresponds to ENOTCONN, WSAENOTCONN.
     */
    WHIST_ERROR_NOT_CONNECTED = -12,
    /**
     * Address is already in use.
     *
     * Returned by a bind operation when the address is already in use
     * by some other socket.
     *
     * Corresponds to EADDRINUSE, WSAEADDRINUSE.
     */
    WHIST_ERROR_ADDRESS_IN_USE = -13,
    /**
     * Address is not available for use by this operation.
     *
     * Returned by an operation which requires a local address but the
     * address provided is not local.
     *
     * Corresponds to EADDRNOTAVAIL, WSAEADDRNOTAVAIL.
     */
    WHIST_ERROR_ADDRESS_NOT_AVAILABLE = -14,
    /**
     * Address is not reachable by this operation.
     *
     * Returned by an operation which requires a remote address but the
     * address provided cannot be contacted.
     *
     * Corresponds to ENETUNREACH, EHOSTUNREACH, WSAENETUNREACH,
     * WSAEHOSTUNREACH.
     */
    WHIST_ERROR_ADDRESS_UNREACHABLE = -15,
    /**
     * Connection was reset by the remote end.
     *
     * Returned by packet operations when the remote end has reset the
     * connection.
     *
     * Corresponds to ECONNRESET, WSAECONNRESET.
     */
    WHIST_ERROR_CONNECTION_RESET = -16,
    /**
     * Connection was refused by the remote end.
     *
     * Returned by a connect operation (for a connected socket) or a
     * packet operation (for a connectionless socket) if there was
     * nothing listening at the remote end.
     */
    WHIST_ERROR_CONNECTION_REFUSED = -17,
    /**
     * No buffers available for operation.
     *
     * May be returned by packet operations like send if no buffer space
     * is available (though the packet may just be dropped instead).
     *
     * Corresponds to ENOBUFS, WSAENOBUFS.
     */
    WHIST_ERROR_NO_BUFFERS = -18,
    /**
     * End-of-file.
     *
     * Corresponds to EOF or zero return from some read functions.
     */
    WHIST_ERROR_END_OF_FILE = -19,
    /**
     * Error in external library.
     *
     * An unspecified error from an external library.  Error codes from
     * external libraries should be mapped to Whist error codes where
     * possible, but when not possible this code can be used to indicate
     * the source of the error without any more detail.
     */
    WHIST_ERROR_EXTERNAL = -20,
} WhistStatus;

/**
 * Retrieve a string representation of the given error code.
 *
 * For use in log messages.  If the error code is not valid it returns a
 * string indicating that.
 *
 * @param error_code  Error code to find.
 * @return  String representing the error code.
 */
const char *whist_error_string(WhistStatus error_code);

/** @} */

#endif /* WHIST_CORE_ERROR_CODES_H */
