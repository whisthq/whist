/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file socket_internal.h
 * @brief Socket wrapper internal interface.
 */
#ifndef WHIST_NETWORK_SOCKET_INTERNAL_H
#define WHIST_NETWORK_SOCKET_INTERNAL_H

#include "socket.h"

struct WhistSocket {
    /**
     * User-provided name for this socket, for debug purposes.
     */
    const char *name;
    /**
     * Unique ID, for debug purposes.
     */
    int id;
    /**
     * Socket implementation being used.
     */
    const WhistSocketImplementation *impl;
    /**
     * Network associated with the implementation.
     *
     * May be NULL if no network information is relevant.
     */
    void *network;
    /**
     * Internal information needed by the implementation.
     */
    void *context;
    /**
     * Last error value returned by a call on this socket.
     */
    int last_error;
    /**
     * Cache of socket option values.
     */
    int options[SOCKET_OPTION_COUNT];
};

struct WhistSocketImplementation {
    const char *name;
    WhistSocketType type;
    size_t context_size;

    void (*global_init)(void);
    void (*global_destroy)(void);

    int (*create)(WhistSocket *sock);
    void (*close)(WhistSocket *sock);

    int (*set_option)(WhistSocket *sock, WhistSocketOption opt, int value);

    int (*bind)(WhistSocket *sock, const struct sockaddr *addr, socklen_t addrlen);
    int (*connect)(WhistSocket *sock, const struct sockaddr *addr, socklen_t addrlen);
    int (*listen)(WhistSocket *sock, int backlog);
    WhistSocket *(*accept)(WhistSocket *sock, struct sockaddr *addr, socklen_t *addrlen,
                           const char *name);

    int (*send)(WhistSocket *sock, const void *buf, size_t len, int flags);
    int (*sendto)(WhistSocket *sock, const void *buf, size_t len, int flags,
                  const struct sockaddr *dest_addr, socklen_t addrlen);

    int (*recv)(WhistSocket *sock, void *buf, size_t len, int flags);
    int (*recvfrom)(WhistSocket *sock, void *buf, size_t len, int flags, struct sockaddr *src_addr,
                    socklen_t *addrlen);

    void (*dump_state)(WhistSocket *sock);
};

/**
 * Create an empty socket structure.
 *
 * This is needed by implementation functions which create new sockets,
 * such as TCP accept().
 */
WhistSocket *socket_create_internal(const WhistSocketImplementation *impl, void *network,
                                    const char *name);

/**
 * TCP implementation using BSD sockets directly.
 */
extern const WhistSocketImplementation socket_implementation_bsd_tcp;

/**
 * UDP implementation using BSD sockets directly.
 */
extern const WhistSocketImplementation socket_implementation_bsd_udp;

/**
 * Self-contained fake UDP implementation for testing.
 */
extern const WhistSocketImplementation socket_implementation_fake_udp;

#endif /* WHIST_NETWORK_SOCKET_INTERNAL_H */
