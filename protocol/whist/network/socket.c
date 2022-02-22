/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file socket.h
 * @brief Socket wrapper implementation.
 */

#include "whist/logging/logging.h"
#include "whist/utils/atomic.h"
#include "socket.h"
#include "socket_internal.h"

// Verbose logging of socket wrapper operations.
#if LOG_SOCKET_OPERATIONS
#define LOG_SOCKET(...) LOG_INFO(__VA_ARGS__)
#else
#define LOG_SOCKET(...) \
    do {                \
    } while (0)
#endif

// String socket address for use in log messages.
// Does not work outside function call arguments.
#define SOCKET_ADDRESS(addr, addrlen) socket_address_string((char[32]){0}, 32, addr, addrlen)

// Fail immediately if the implementation does not support the function.
#define SOCKET_CHECK_IMPL(sock, func)                                                              \
    do {                                                                                           \
        if (!sock->impl->func) {                                                                   \
            LOG_FATAL("Invalid function %s called on socket %d (%s) of type %s.", #func, sock->id, \
                      sock->name, socket_type_string(sock->impl->type));                           \
        }                                                                                          \
    } while (0)

static const char *socket_type_string(WhistSocketType type) {
    static const char *type_list[] = {
        [SOCKET_TYPE_UDP] = "UDP",
        [SOCKET_TYPE_TCP] = "TCP",
    };
    if (type >= 0 && type < ARRAY_LENGTH(type_list))
        return type_list[type];
    else
        return "invalid socket type";
}

static const char *socket_option_string(WhistSocketOption opt) {
    static const char *option_list[] = {
        [SOCKET_OPTION_CLOSE_ON_EXEC] = "close-on-exec",
        [SOCKET_OPTION_NON_BLOCKING] = "non-blocking",
        [SOCKET_OPTION_REUSE_ADDRESS] = "reuse address",
        [SOCKET_OPTION_NO_INTERRUPT] = "no interrupt",
        [SOCKET_OPTION_RECEIVE_TIMEOUT] = "receive timeout",
        [SOCKET_OPTION_SEND_TIMEOUT] = "send timeout",
        [SOCKET_OPTION_RECEIVE_BUFFER_SIZE] = "receive buffer size",
        [SOCKET_OPTION_SEND_BUFFER_SIZE] = "send buffer size",
        [SOCKET_OPTION_TYPE_OF_SERVICE] = "type of service",
    };
    if (opt >= 0 && opt < ARRAY_LENGTH(option_list))
        return option_list[opt];
    else
        return "invalid option";
}

const char *socket_error_string(int error) {
    static const char *error_strings[] = {
        [SOCKET_ERROR_NONE] = "success",
        [SOCKET_ERROR_UNKNOWN] = "unknown error",
        [SOCKET_ERROR_INVALID_ARGUMENT] = "invalid argument",
        [SOCKET_ERROR_WOULD_BLOCK] = "operation would block",
        [SOCKET_ERROR_IN_PROGRESS] = "operation in progress",
        [SOCKET_ERROR_NOT_CONNECTED] = "socket not connected",
        [SOCKET_ERROR_ADDRESS_IN_USE] = "address in use",
        [SOCKET_ERROR_ADDRESS_NOT_AVAILABLE] = "address not available",
        [SOCKET_ERROR_ADDRESS_UNREACHABLE] = "address unreachable",
        [SOCKET_ERROR_CONNECTION_RESET] = "connection reset",
        [SOCKET_ERROR_CONNECTION_REFUSED] = "connection refused",
        [SOCKET_ERROR_NO_BUFFERS] = "no buffers",
    };
    if (error >= 0 && error < (int)ARRAY_LENGTH(error_strings))
        return error_strings[error];
    else
        return "invalid error code";
}

#if LOG_SOCKET_OPERATIONS
static char *socket_address_string(char *buf, size_t len, const struct sockaddr *addr,
                                   socklen_t addrlen) {
    if (addr->sa_family == AF_INET) {
        if (addrlen < sizeof(struct sockaddr_in)) {
            snprintf(buf, len, "invalid IPv4 address");
        } else {
            const struct sockaddr_in *ipv4_addr = (const struct sockaddr_in *)addr;
            const uint8_t *ip = (const uint8_t *)&ipv4_addr->sin_addr.s_addr;
            snprintf(buf, len, "%d.%d.%d.%d:%d", ip[0], ip[1], ip[2], ip[3],
                     ntohs(ipv4_addr->sin_port));
        }
    } else {
        // IPv6, UNIX could be included here if needed.
        snprintf(buf, len, "unknown address family %d", addr->sa_family);
    }
    return buf;
}
#endif

static atomic_int socket_unique_id = ATOMIC_VAR_INIT(1);

WhistSocket *socket_create_internal(const WhistSocketImplementation *impl, void *network,
                                    const char *name) {
    WhistSocket *sock;

    sock = safe_malloc(sizeof(*sock));
    memset(sock, 0, sizeof(*sock));

    if (name) {
        sock->name = name;
    } else {
        // Hint to people that they should set names.
        sock->name = "naughty developer did not set a name for this socket";
    }
    sock->id = atomic_fetch_add(&socket_unique_id, 1);
    sock->impl = impl;
    sock->network = network;

    sock->context = safe_malloc(impl->context_size);
    memset(sock->context, 0, impl->context_size);

    return sock;
}

static struct {
    const WhistSocketImplementation *impl;
    void *network;
} socket_defaults[] = {
    [SOCKET_TYPE_UDP] = {&socket_implementation_bsd_udp, NULL},
    [SOCKET_TYPE_TCP] = {&socket_implementation_bsd_tcp, NULL},
};

void socket_set_default_implementation(WhistSocketType type, const WhistSocketImplementation *impl,
                                       void *network) {
    FATAL_ASSERT(type >= 0 && type < (int)ARRAY_LENGTH(socket_defaults));
    socket_defaults[type].impl = impl;
    socket_defaults[type].network = network;
}

WhistSocket *socket_create(WhistSocketType type, const char *name,
                           const WhistSocketImplementation *impl, void *network) {
    WhistSocket *sock;
    int ret;

    if (!impl) {
        // Pick implementation from type.
        if (type >= 0 && type < ARRAY_LENGTH(socket_defaults)) {
            impl = socket_defaults[type].impl;
            network = socket_defaults[type].network;
        }
        if (!impl) {
            // Invalid type or no default.
            return NULL;
        }
    }

    sock = socket_create_internal(impl, network, name);
    if (!sock) {
        return NULL;
    }

    ret = impl->create(sock);
    if (ret < 0) {
        free(sock->context);
        free(sock);
        return NULL;
    }

    LOG_SOCKET("Socket %d (%s) created as %s using %s.", sock->id, sock->name,
               socket_type_string(type), impl->name);
    return sock;
}

void socket_close(WhistSocket *sock) {
    LOG_SOCKET("Socket %d (%s) closing.", sock->id, sock->name);
    sock->impl->close(sock);
    free(sock->context);
    free(sock);
}

int socket_get_last_error(WhistSocket *sock) { return sock->last_error; }

int socket_set_option(WhistSocket *sock, WhistSocketOption opt, int value) {
    SOCKET_CHECK_IMPL(sock, set_option);
    int ret;
    if (opt >= 0 && opt < SOCKET_OPTION_COUNT) {
        ret = sock->impl->set_option(sock, opt, value);
    } else {
        ret = -1;
    }
    if (ret < 0) {
        LOG_SOCKET("Socket %d (%s) failed to set option %s with value %d: %s.", sock->id,
                   sock->name, socket_option_string(opt), value,
                   socket_error_string(sock->last_error));
    } else {
        LOG_SOCKET("Socket %d (%s) set option %s with value %d.", sock->id, sock->name,
                   socket_option_string(opt), value);
    }
    return ret;
}

int socket_get_option(WhistSocket *sock, WhistSocketOption opt, int *value) {
    if (opt >= 0 && opt < SOCKET_OPTION_COUNT) {
        *value = sock->options[opt];
        LOG_SOCKET("Socket %d (%s) got option %s with value %d.", sock->id, sock->name,
                   socket_option_string(opt), *value);
        return 0;
    } else {
        LOG_SOCKET("Socket %d (%s) failed to get option %s.", sock->id, sock->name,
                   socket_option_string(opt));
        sock->last_error = SOCKET_ERROR_INVALID_ARGUMENT;
        return -1;
    }
}

int socket_bind(WhistSocket *sock, const struct sockaddr *addr, socklen_t addrlen) {
    SOCKET_CHECK_IMPL(sock, bind);
    int ret = sock->impl->bind(sock, addr, addrlen);
    if (ret < 0) {
        LOG_SOCKET("Socket %d (%s) failed to bind to %s: %s.", sock->id, sock->name,
                   SOCKET_ADDRESS(addr, addrlen), socket_error_string(sock->last_error));
    } else {
        LOG_SOCKET("Socket %d (%s) bound to %s.", sock->id, sock->name,
                   SOCKET_ADDRESS(addr, addrlen));
    }
    return ret;
}

int socket_connect(WhistSocket *sock, const struct sockaddr *addr, socklen_t addrlen) {
    SOCKET_CHECK_IMPL(sock, connect);
    int ret = sock->impl->connect(sock, addr, addrlen);
    if (ret < 0) {
        LOG_SOCKET("Socket %d (%s) failed to connect to %s: %s.", sock->id, sock->name,
                   SOCKET_ADDRESS(addr, addrlen), socket_error_string(sock->last_error));
    } else {
        LOG_SOCKET("Socket %d (%s) connected to %s.", sock->id, sock->name,
                   SOCKET_ADDRESS(addr, addrlen));
    }
    return ret;
}

int socket_listen(WhistSocket *sock, int backlog) {
    SOCKET_CHECK_IMPL(sock, listen);
    int ret = sock->impl->listen(sock, backlog);
    if (ret < 0) {
        LOG_SOCKET("Socket %d (%s) failed to listen for %d connections: %s.", sock->id, sock->name,
                   backlog, socket_error_string(sock->last_error));
    } else {
        LOG_SOCKET("Socket %d (%s) listening for %d connections.", sock->id, sock->name, backlog);
    }
    return ret;
}

WhistSocket *socket_accept(WhistSocket *sock, struct sockaddr *addr, socklen_t *addrlen,
                           const char *name) {
    SOCKET_CHECK_IMPL(sock, accept);
    WhistSocket *tmp = sock->impl->accept(sock, addr, addrlen, name);
    if (tmp) {
        LOG_SOCKET("Socket %d (%s) accepted %s socket %d (%s) as %s.", sock->id, sock->name,
                   socket_type_string(tmp->impl->type), tmp->id, tmp->name, tmp->impl->name);
    } else {
        LOG_SOCKET("Socket %d (%s) failed to accept %s socket: %s.", sock->id, sock->name,
                   socket_type_string(tmp->impl->type), socket_error_string(sock->last_error));
    }
    return tmp;
}

int socket_send(WhistSocket *sock, const void *buf, size_t len, int flags) {
    SOCKET_CHECK_IMPL(sock, send);
    int ret = sock->impl->send(sock, buf, len, flags);
    if (ret < 0) {
        LOG_SOCKET("Socket %d (%s) failed to send %zu bytes: %s.", sock->id, sock->name, len,
                   socket_error_string(sock->last_error));
    } else {
        LOG_SOCKET("Socket %d (%s) sent %d/%zu bytes.", sock->id, sock->name, ret, len);
    }
    return ret;
}

int socket_sendto(WhistSocket *sock, const void *buf, size_t len, int flags,
                  const struct sockaddr *dest_addr, socklen_t addrlen) {
    SOCKET_CHECK_IMPL(sock, sendto);
    int ret = sock->impl->sendto(sock, buf, len, flags, dest_addr, addrlen);
    if (ret < 0) {
        LOG_SOCKET("Socket %d (%s) failed to send %zu bytes to %s: %s.", sock->id, sock->name, len,
                   SOCKET_ADDRESS(dest_addr, addrlen), socket_error_string(sock->last_error));
    } else {
        LOG_SOCKET("Socket %d (%s) sent %d/%zu bytes to %s.", sock->id, sock->name, ret, len,
                   SOCKET_ADDRESS(dest_addr, addrlen));
    }
    return ret;
}

int socket_recv(WhistSocket *sock, void *buf, size_t len, int flags) {
    SOCKET_CHECK_IMPL(sock, recv);
    int ret = sock->impl->recv(sock, buf, len, flags);
    if (ret < 0) {
        LOG_SOCKET("Socket %d (%s) failed to receive %zu bytes: %s.", sock->id, sock->name, len,
                   socket_error_string(sock->last_error));
    } else {
        LOG_SOCKET("Socket %d (%s) received %d/%zu bytes.", sock->id, sock->name, ret, len);
    }
    return ret;
}

int socket_recvfrom(WhistSocket *sock, void *buf, size_t len, int flags, struct sockaddr *src_addr,
                    socklen_t *addrlen) {
    SOCKET_CHECK_IMPL(sock, recvfrom);
    int ret = sock->impl->recvfrom(sock, buf, len, flags, src_addr, addrlen);
    if (ret < 0) {
        LOG_SOCKET("Socket %d (%s) failed to receive %zu bytes: %s.", sock->id, sock->name, len,
                   socket_error_string(sock->last_error));
    } else {
        LOG_SOCKET("Socket %d (%s) received %d/%zu bytes from %s.", sock->id, sock->name, ret, len,
                   SOCKET_ADDRESS(src_addr, *addrlen));
    }
    return ret;
}

void socket_dump_state(WhistSocket *sock) {
    LOG_INFO("Socket %d (%s):", sock->id, sock->name);
    LOG_INFO("  Type %d (%s) with implementation %s", sock->impl->type,
             socket_type_string(sock->impl->type), sock->impl->name);
    LOG_INFO("  Last error %d (%s)", sock->last_error, socket_error_string(sock->last_error));
    LOG_INFO("  Options:");
    for (int i = 0; i < SOCKET_OPTION_COUNT; i++) {
        LOG_INFO("    %s = %d", socket_option_string(i), sock->options[i]);
    }
    if (sock->impl->dump_state) {
        LOG_INFO("  Implementation state:");
        sock->impl->dump_state(sock);
    }
}
