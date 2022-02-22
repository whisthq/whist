/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file socket.h
 * @brief Socket wrapper external interface.
 */
#ifndef WHIST_NETWORK_SOCKET_H
#define WHIST_NETWORK_SOCKET_H

typedef struct WhistSocket WhistSocket;
typedef struct WhistSocketImplementation WhistSocketImplementation;

/**
 * Type enumerating protocols usable with WhistSockets.
 */
typedef enum {
    SOCKET_TYPE_UDP,
    SOCKET_TYPE_TCP,
} WhistSocketType;

/**
 * Create a new socket.
 *
 * @param type     Socket type to create.
 * @param name     Developer-friendly name for the socket.
 * @param impl     Use the given socket implementation, or set as NULL
 *                 to use the default.
 * @param context  Network associated with the implementation.  Ignored
 *                 impl is not set.
 * @return  New socket instance, or NULL on failure.
 */
WhistSocket *socket_create(WhistSocketType type, const char *name,
                           const WhistSocketImplementation *impl, void *network);

/**
 * Close a socket.
 *
 * @param sock  Socket to close.
 */
void socket_close(WhistSocket *sock);

enum {
    /**
     * Not an error.
     *
     * Yay!
     */
    SOCKET_ERROR_NONE = 0,
    /**
     * Something unknown went wrong.
     */
    SOCKET_ERROR_UNKNOWN,
    /**
     * Invalid argument.
     *
     * Corresponds to EINVAL, WSAEINVAL.
     */
    SOCKET_ERROR_INVALID_ARGUMENT,
    /**
     * Operation would block for longer than wanted.
     *
     * Returned for non-blocking sockets when the operation is unable to
     * complete immediately, and also when a timeout is set and elapses.
     *
     * Corresponds to EAGAIN, EWOULDBLOCK, WSAWOULDBLOCK.
     */
    SOCKET_ERROR_WOULD_BLOCK,
    /**
     * Operation has started but not finished.
     *
     * Returned by a connect operation.
     *
     * Corresponds to EINPROGRESS, WSAWOULDBLOCK.
     */
    SOCKET_ERROR_IN_PROGRESS,
    /**
     * Operation requires a connection but doesn't have one.
     *
     * Returned by packet operations like send on an unconnected socket.
     *
     * Corresponds to ENOTCONN, WSAENOTCONN.
     */
    SOCKET_ERROR_NOT_CONNECTED,
    /**
     * Address is already in use.
     *
     * Returned by a bind operation when the address is already in use
     * by some other socket.
     *
     * Corresponds to EADDRINUSE, WSAEADDRINUSE.
     */
    SOCKET_ERROR_ADDRESS_IN_USE,
    /**
     * Address is not available for use by this operation.
     *
     * Returned by an operation which requires a local address but the
     * address provided is not local.
     *
     * Corresponds to EADDRNOTAVAIL, WSAEADDRNOTAVAIL.
     */
    SOCKET_ERROR_ADDRESS_NOT_AVAILABLE,
    /**
     * Address is not reachable by this operation.
     *
     * Returned by an operation which requires a remote address but the
     * address provided cannot be contacted.
     *
     * Corresponds to ENETUNREACH, EHOSTUNREACH, WSAENETUNREACH,
     * WSAEHOSTUNREACH.
     */
    SOCKET_ERROR_ADDRESS_UNREACHABLE,
    /**
     * Connection was reset by the remote end.
     *
     * Returned by packet operations when the remote end has reset the
     * connection.
     *
     * Corresponds to ECONNRESET, WSAECONNRESET.
     */
    SOCKET_ERROR_CONNECTION_RESET,
    /**
     * Connection was refused by the remote end.
     *
     * Returned by a connect operation (for a connected socket) or a
     * packet operation (for a connectionless socket) if there was
     * nothing listening at the remote end.
     */
    SOCKET_ERROR_CONNECTION_REFUSED,
    /**
     * No buffers available for operation.
     *
     * May be returned by packet operations like send if no buffer space
     * is available (though the packet may just be dropped instead).
     *
     * Corresponds to ENOBUFS, WSAENOBUFS.
     */
    SOCKET_ERROR_NO_BUFFERS,
};

/**
 * Get a string representation of an error number.
 *
 * The string returned is static and read-only.
 *
 * @param error  Error number to get string for.
 * @return  String representation of error number.
 */
const char *socket_error_string(int error);

/**
 * Get the most recent error on socket.
 *
 * @param sock  Socket to query.
 * @return  Most recent error value, or zero if none.
 */
int socket_get_last_error(WhistSocket *sock);

/**
 * Type enumerating possible socket options.
 */
typedef enum {
    /**
     * Close-on-exec option (O_CLOEXEC).
     *
     * Value is boolean.
     */
    SOCKET_OPTION_CLOSE_ON_EXEC,
    /**
     * Non-blocking option (O_NONBLOCK).
     *
     * Value is boolean.
     */
    SOCKET_OPTION_NON_BLOCKING,
    /**
     * Re-use address option (SO_REUSEADDR).
     *
     * Value is boolean.
     */
    SOCKET_OPTION_REUSE_ADDRESS,
    /**
     * No interrupt option.
     *
     * When set, blocking send and receive calls will be restarted if an
     * interrupt happens rather than returning to the caller.
     *
     * Value is boolean.
     */
    SOCKET_OPTION_NO_INTERRUPT,
    /**
     * Receive timeout option (SO_RCVTIMEO).
     *
     * Value is a timeout in milliseconds.
     */
    SOCKET_OPTION_RECEIVE_TIMEOUT,
    /**
     * Send timeout option (SO_SNDTIMEO).
     *
     * Value is a timeout in milliseconds.
     */
    SOCKET_OPTION_SEND_TIMEOUT,
    /**
     * Receive buffer size option (SO_RCVBUF).
     *
     * Value is a buffer size in bytes.
     */
    SOCKET_OPTION_RECEIVE_BUFFER_SIZE,
    /**
     * Send buffer size option (SO_SNDBUF).
     *
     * Value is a buffer size in bytes.
     */
    SOCKET_OPTION_SEND_BUFFER_SIZE,
    /**
     * IP type-of-service field (IP_TOS).
     *
     * Value is a byte for the IP TOS field.  This should set only the
     * DSCP field, so the bottom two bits should be zero.
     */
    SOCKET_OPTION_TYPE_OF_SERVICE,
    /**
     * Number of different socket options.
     */
    SOCKET_OPTION_COUNT,
} WhistSocketOption;

/**
 * Set an option value on a socket.
 *
 * Equivalent to some combination of fnctl(), setsockopt() and ioctl(),
 * depending on the option being set.
 *
 * @param sock   Socket to set option value on.
 * @param opt    Option type to set.
 * @param value  Value associated with the options.  The interpretation
 *               of the value depends on the option type.
 * @return  Zero on success or minus one on failure.
 */
int socket_set_option(WhistSocket *sock, WhistSocketOption opt, int value);

/**
 * Get an option value from a socket.
 *
 * @param sock   Socket to get option value from.
 * @param opt    Option type to get.
 * @param value  Written with the value associated with the option on
 *               success.
 * @return  Zero on success or minus one on failure.
 */
int socket_get_option(WhistSocket *sock, WhistSocketOption opt, int *value);

/**
 * Bind a socket to an address.
 *
 * Equivalent to bind().
 *
 * @param sock     Socket to bind.
 * @param addr     Local address to bind to.
 * @param addrlen  Length of the addr argument.
 * @return  Zero on success or minus one on failure.
 */
int socket_bind(WhistSocket *sock, const struct sockaddr *addr, socklen_t addrlen);

/**
 * Connect a socket, or set the default destination.
 *
 * Equivalent to connect().
 *
 * @param sock     Socket to connect.
 * @param addr     Remote address to connect to.
 * @param addrlen  Length of the addr argument.
 * @return  Zero on success or minus one on failure.
 */
int socket_connect(WhistSocket *sock, const struct sockaddr *addr, socklen_t addrlen);

/**
 * Start listening for connections on a socket.
 *
 * Equivalent to listen().
 *
 * @param sock     Socket to start listening on.
 * @param backlog  Maximum number of pending connections allowed.
 * @return  Zero on success or minus one on failure.
 */
int socket_listen(WhistSocket *sock, int backlog);

/**
 * Accept a connection on a listening socket, making a new connected
 * socket.
 *
 * Equivalent to accept().
 *
 * @param sock     Sock to accept a connection on.
 * @param addr     Remote address the connection came from.
 * @param addrlen  Length of the returned addr field.
 * @param name     Developer-friendly name to associated with the new
 *                 connected socket.
 * @return  The new connected socket on success, or NULL on failure.
 */
WhistSocket *socket_accept(WhistSocket *sock, struct sockaddr *addr, socklen_t *addrlen,
                           const char *name);

/**
 * Send data on a connected socket.
 *
 * Equivalent to send().
 *
 * @param sock   Socket to send data on.
 * @param buf    Buffer containing data to send.
 * @param len    Number of bytes to send.
 * @param flags  Flags to use with this send operation.
 * @return  The number of bytes sent, or minus one on failure.
 */
int socket_send(WhistSocket *sock, const void *buf, size_t len, int flags);

/**
 * Send data to a given address on a socket.
 *
 * Equivalent to sendto().
 *
 * @param sock       Socket to send data on.
 * @param buf        Buffer containing data to send.
 * @param len        Number of bytes to send.
 * @param flags      Flags to use with this send operation.
 * @param dest_addr  Address to send to.
 * @param addrlen    Length of the dest_addr argument.
 * @return  The number of bytes sent, or minus one on failure.
 */
int socket_sendto(WhistSocket *sock, const void *buf, size_t len, int flags,
                  const struct sockaddr *dest_addr, socklen_t addrlen);

/**
 * Receive data from a socket.
 *
 * Equivalent to recv().
 *
 * @param sock   Socket to receive data on.
 * @param buf    Buffer to write received data to.
 * @param len    Maximum number of bytes to receive.
 * @param flags  Flags to use with this receive operation.
 * @return  The number of byte received, or minus one on failure.
 */
int socket_recv(WhistSocket *sock, void *buf, size_t len, int flags);

/**
 * Receive data from a socket, noting where it came from.
 *
 * Equivalent to recvfrom().
 *
 * @param sock      Socket to receive data on.
 * @param buf       Buffer to write received data to.
 * @param len       Maximum number of bytes to receive.
 * @param flags     Flags to use with this receive operation.
 * @param src_addr  Remove address that the data came from.
 * @param addrlen   Length of the returned src_addr field.
 * @return  The number of byte received, or minus one on failure.
 */
int socket_recvfrom(WhistSocket *sock, void *buf, size_t len, int flags, struct sockaddr *src_addr,
                    socklen_t *addrlen);

/**
 * Dump all socket state to the log.
 *
 * For debugging purposes.
 *
 * @param sock  Socket to dump state for.
 */
void socket_dump_state(WhistSocket *sock);

/**
 * Set the default implementation for a socket type.
 *
 * For test purposes, where the component being tested does not allow
 * the implementation to be set directly.
 *
 * @param type     Type to set default implementation for.
 * @param impl     Implementation to use.
 * @param context  Network to use with the implementation.
 */
void socket_set_default_implementation(WhistSocketType type, const WhistSocketImplementation *impl,
                                       void *network);

#endif /* WHIST_NETWORK_SOCKET_H */
