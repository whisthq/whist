/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file socket.h
 * @brief BSD sockets implementation for socket API.
 */

#ifdef _WIN32
// Winsock.
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#endif

#include "whist/core/whist.h"
#include "socket.h"
#include "socket_internal.h"

typedef struct {
#ifdef _WIN32
    SOCKET fd;
#else
    int fd;
#endif
} BSDSocketContext;

static void bsd_socket_global_init(void) {
#ifdef _WIN32
    WSADATA data;
    int ret = WSAStartup(MAKEWORD(2, 2), &data);
    if (ret) {
        LOG_FATAL("WSAStartup() failed: %d.", WSAGetLastError());
    }
#endif
}

static void bsd_socket_global_destroy(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

static int bsd_socket_create(WhistSocket *sock) {
    BSDSocketContext *ctx = sock->context;

    int domain, type, proto;

    if (sock->impl->type == SOCKET_TYPE_UDP) {
        domain = AF_INET;
        type = SOCK_DGRAM;
        proto = IPPROTO_UDP;
    } else if (sock->impl->type == SOCKET_TYPE_TCP) {
        domain = AF_INET;
        type = SOCK_STREAM;
        proto = IPPROTO_TCP;
    } else {
        return -1;
    }

    // Set CLOEXEC on creation if possible to avoid the socket being
    // inherited by child processes.  If this is not possible then we
    // will set it afterwards (with a rare benign race condition).
#ifdef SOCK_CLOEXEC
    type |= SOCK_CLOEXEC;
#endif

    ctx->fd = socket(domain, type, proto);

    if (ctx->fd < 0) {
        // Failed to create the socket.
        return -1;
    }

#ifndef _WIN32
    int flags = fcntl(ctx->fd, F_GETFD);
    if (flags >= 0 && !(flags & FD_CLOEXEC)) {
        fcntl(ctx->fd, F_SETFD, flags | FD_CLOEXEC);
    }
    sock->options[SOCKET_OPTION_CLOSE_ON_EXEC] = 1;

    // Set the no-interrupt option by default.
    sock->options[SOCKET_OPTION_NO_INTERRUPT] = 1;
#endif

    // Fetch defaults for buffer sizes.
    int ret, value;
    socklen_t len;

    len = sizeof(value);
    ret = getsockopt(ctx->fd, SOL_SOCKET, SO_RCVBUF, (void *)&value, &len);
    if (ret < 0) {
        value = 0;
    }
    sock->options[SOCKET_OPTION_RECEIVE_BUFFER_SIZE] = value;

    len = sizeof(value);
    ret = getsockopt(ctx->fd, SOL_SOCKET, SO_SNDBUF, (void *)&value, &len);
    if (ret < 0) {
        value = 0;
    }
    sock->options[SOCKET_OPTION_SEND_BUFFER_SIZE] = value;

    return 0;
}

static void bsd_socket_close(WhistSocket *sock) {
    BSDSocketContext *ctx = sock->context;

#ifdef _WIN32
    closesocket(ctx->fd);
#else
    close(ctx->fd);
#endif
}

typedef struct {
    int system;
    int whist;
} BSDSocketError;

static const BSDSocketError bsd_socket_error_map[] = {
#ifdef _WIN32
    {WSAEINVAL, SOCKET_ERROR_INVALID_ARGUMENT},
    {WSAEWOULDBLOCK, SOCKET_ERROR_WOULD_BLOCK},
    {WSAETIMEDOUT, SOCKET_ERROR_WOULD_BLOCK},
    {WSAENOTCONN, SOCKET_ERROR_NOT_CONNECTED},
    {WSAEADDRINUSE, SOCKET_ERROR_ADDRESS_IN_USE},
    {WSAEADDRNOTAVAIL, SOCKET_ERROR_ADDRESS_NOT_AVAILABLE},
    {WSAENETUNREACH, SOCKET_ERROR_ADDRESS_UNREACHABLE},
    {WSAEHOSTUNREACH, SOCKET_ERROR_ADDRESS_UNREACHABLE},
    {WSAECONNRESET, SOCKET_ERROR_CONNECTION_RESET},
    {WSAECONNREFUSED, SOCKET_ERROR_CONNECTION_REFUSED},
    {WSAENOBUFS, SOCKET_ERROR_NO_BUFFERS},
#else
    {EINVAL, SOCKET_ERROR_INVALID_ARGUMENT},
    {EAGAIN, SOCKET_ERROR_WOULD_BLOCK},
    {EWOULDBLOCK, SOCKET_ERROR_WOULD_BLOCK},
    {EINPROGRESS, SOCKET_ERROR_IN_PROGRESS},
    {ENOTCONN, SOCKET_ERROR_NOT_CONNECTED},
    {EADDRINUSE, SOCKET_ERROR_ADDRESS_IN_USE},
    {EADDRNOTAVAIL, SOCKET_ERROR_ADDRESS_NOT_AVAILABLE},
    {ENETUNREACH, SOCKET_ERROR_ADDRESS_UNREACHABLE},
    {EHOSTUNREACH, SOCKET_ERROR_ADDRESS_UNREACHABLE},
    {ECONNRESET, SOCKET_ERROR_CONNECTION_RESET},
    {ECONNREFUSED, SOCKET_ERROR_CONNECTION_REFUSED},
    {ENOBUFS, SOCKET_ERROR_NO_BUFFERS},
#endif
};

static int bsd_socket_check_error(WhistSocket *sock, int ret) {
    BSDSocketContext *ctx = sock->context;

    if (ret < 0) {
        int error;
#ifdef _WIN32
        error = WSAGetLastError();
#else
        error = errno;
#endif
        for (size_t k = 0; k < ARRAY_LENGTH(bsd_socket_error_map); k++) {
            const BSDSocketError *map = &bsd_socket_error_map[k];
            if (map->system == error) {
                sock->last_error = map->whist;
                return -1;
            }
        }
        sock->last_error = SOCKET_ERROR_UNKNOWN;
        return -1;
    } else {
        sock->last_error = 0;
        return ret;
    }
}

#ifndef _WIN32
static int bsd_socket_set_fcntl_option(int fd, int get, int set, int flag, int value) {
    int ret = fcntl(fd, get);
    if (ret < 0) {
        // Unable to set value.
        return -1;
    }

    if ((ret & flag) == (value ? flag : 0)) {
        // Already set to the correct value.
        return 0;
    }

    return fcntl(fd, set, ret ^ flag);
}
#else
static int bsd_socket_set_ioctl_option(int fd, int cmd, int value) {
    u_long arg = value;
    return ioctlsocket(fd, cmd, &arg);
}
#endif

static int bsd_socket_set_option(WhistSocket *sock, WhistSocketOption opt, int value) {
    BSDSocketContext *ctx = sock->context;
    int ret;

    switch (opt) {
        case SOCKET_OPTION_CLOSE_ON_EXEC:
#ifdef _WIN32
            // No effect on Windows, call that success.
            ret = 0;
#else
            ret = bsd_socket_set_fcntl_option(ctx->fd, F_GETFD, F_SETFD, FD_CLOEXEC, value);
#endif
            break;
        case SOCKET_OPTION_NON_BLOCKING:
#ifdef _WIN32
            ret = bsd_socket_set_ioctl_option(ctx->fd, FIONBIO, value);
#else
            ret = bsd_socket_set_fcntl_option(ctx->fd, F_GETFL, F_SETFL, O_NONBLOCK, value);
#endif
            break;
        case SOCKET_OPTION_REUSE_ADDRESS:
            ret =
                setsockopt(ctx->fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&value, sizeof(value));
            break;
        case SOCKET_OPTION_NO_INTERRUPT:
            // No extra socket setting.
            ret = 0;
            break;
        case SOCKET_OPTION_RECEIVE_TIMEOUT:
        case SOCKET_OPTION_SEND_TIMEOUT: {
            bool recv = opt == SOCKET_OPTION_RECEIVE_TIMEOUT;
            // setsockopt(SO_RCVTIMEO / SO_SNDTIMEO) argument is struct
            // timeval on Unices but DWORD in milliseconds on Windows.
#ifdef _WIN32
            DWORD timeout = value;
#else
            struct timeval timeout = {.tv_sec = value / MS_IN_SECOND,
                                      .tv_usec = value % MS_IN_SECOND * US_IN_MS};
#endif
            ret = setsockopt(ctx->fd, SOL_SOCKET, recv ? SO_RCVTIMEO : SO_SNDTIMEO,
                             (const void *)&timeout, sizeof(timeout));
            break;
        }
        case SOCKET_OPTION_RECEIVE_BUFFER_SIZE:
            ret = setsockopt(ctx->fd, SOL_SOCKET, SO_RCVBUF, (const void *)&value, sizeof(value));
            break;
        case SOCKET_OPTION_SEND_BUFFER_SIZE:
            ret = setsockopt(ctx->fd, SOL_SOCKET, SO_SNDBUF, (const void *)&value, sizeof(value));
            break;
        case SOCKET_OPTION_TYPE_OF_SERVICE:
            ret = setsockopt(ctx->fd, IPPROTO_IP, IP_TOS, (const char *)&value, sizeof(value));
            break;
        default:
            // Unknown option.
            sock->last_error = SOCKET_ERROR_INVALID_ARGUMENT;
            return -1;
    }
    if (ret == 0) {
        sock->options[opt] = value;
    }
    return bsd_socket_check_error(sock, ret);
}

static int bsd_socket_bind(WhistSocket *sock, const struct sockaddr *addr, socklen_t addrlen) {
    BSDSocketContext *ctx = sock->context;
    int ret;

    ret = bind(ctx->fd, addr, addrlen);

    return bsd_socket_check_error(sock, ret);
}

static int bsd_socket_connect(WhistSocket *sock, const struct sockaddr *addr, socklen_t addrlen) {
    BSDSocketContext *ctx = sock->context;
    int ret;

    ret = connect(ctx->fd, addr, addrlen);

#ifdef _WIN32
    // Fix up nonstandard Windows connect() error code.
    if (ret == -1 && WSAGetLastError() == WSAEWOULDBLOCK) {
        sock->last_error = SOCKET_ERROR_IN_PROGRESS;
        return -1;
    }
#endif

    return bsd_socket_check_error(sock, ret);
}

static int bsd_socket_listen(WhistSocket *sock, int backlog) {
    BSDSocketContext *ctx = sock->context;
    int ret;

    ret = listen(ctx->fd, backlog);

    return bsd_socket_check_error(sock, ret);
}

static WhistSocket *bsd_socket_accept(WhistSocket *sock, struct sockaddr *addr, socklen_t *addrlen,
                                      const char *name) {
    BSDSocketContext *ctx = sock->context;
    int ret;

    ret = accept(ctx->fd, addr, addrlen);

    if (ret < 0) {
        bsd_socket_check_error(sock, ret);
        return NULL;
    }

    WhistSocket *new_sock = socket_create_internal(&socket_implementation_bsd_tcp, NULL, name);
    BSDSocketContext *new_ctx = new_sock->context;
    *new_ctx = (BSDSocketContext){.fd = ret};
    return new_sock;
}

static int bsd_socket_send(WhistSocket *sock, const void *buf, size_t len, int flags) {
    BSDSocketContext *ctx = sock->context;
    int ret;

#ifdef _WIN32
    ret = send(ctx->fd, buf, (int)len, flags);
#else
    ret = send(ctx->fd, buf, len, flags);
#endif

    return bsd_socket_check_error(sock, ret);
}

static int bsd_socket_sendto(WhistSocket *sock, const void *buf, size_t len, int flags,
                             const struct sockaddr *dest_addr, socklen_t addrlen) {
    BSDSocketContext *ctx = sock->context;
    int ret;

#ifdef _WIN32
    ret = sendto(ctx->fd, buf, (int)len, flags, dest_addr, addrlen);
#else
    ret = sendto(ctx->fd, buf, len, flags, dest_addr, addrlen);
#endif

    return bsd_socket_check_error(sock, ret);
}

static int bsd_socket_recv(WhistSocket *sock, void *buf, size_t len, int flags) {
    BSDSocketContext *ctx = sock->context;
    int ret;

    // This also avoids returning EINTR, since that is never useful in
    // our current implementation.
#ifdef _WIN32
    // EINTR doesn't happen on windows, so don't need anything else here.
    ret = recv(ctx->fd, buf, (int)len, flags);
#else
    // TODO: recv_no_intr() can be expanded here once it has no other
    // callers.
    ret = recv_no_intr(ctx->fd, buf, len, flags);
#endif

    return bsd_socket_check_error(sock, ret);
}

static int bsd_socket_recvfrom(WhistSocket *sock, void *buf, size_t len, int flags,
                               struct sockaddr *src_addr, socklen_t *addrlen) {
    BSDSocketContext *ctx = sock->context;
    int ret;

    // This also avoids returning EINTR, since that is never useful in
    // our current implementation.
#ifdef _WIN32
    // EINTR doesn't happen on windows, so don't need anything else here.
    ret = recvfrom(ctx->fd, buf, (int)len, flags, src_addr, addrlen);
#else
    // TODO: recvfrom_no_intr() can be expanded here once it has no
    // other callers.
    ret = recvfrom_no_intr(ctx->fd, buf, len, flags, src_addr, addrlen);
#endif

    return bsd_socket_check_error(sock, ret);
}

static void bsd_socket_dump_state(WhistSocket *sock) {
    BSDSocketContext *ctx = sock->context;
    LOG_INFO("    fd %d", ctx->fd);
}

const WhistSocketImplementation socket_implementation_bsd_tcp = {
    .name = "BSD TCP socket",
    .type = SOCKET_TYPE_TCP,
    .context_size = sizeof(BSDSocketContext),

    .global_init = &bsd_socket_global_init,
    .global_destroy = &bsd_socket_global_destroy,

    .create = &bsd_socket_create,
    .close = &bsd_socket_close,

    .set_option = &bsd_socket_set_option,

    .bind = &bsd_socket_bind,
    .connect = &bsd_socket_connect,
    .listen = &bsd_socket_listen,
    .accept = &bsd_socket_accept,

    .send = &bsd_socket_send,
    .recv = &bsd_socket_recv,

    .dump_state = &bsd_socket_dump_state,
};

const WhistSocketImplementation socket_implementation_bsd_udp = {
    .name = "BSD UDP socket",
    .type = SOCKET_TYPE_UDP,
    .context_size = sizeof(BSDSocketContext),

    .global_init = &bsd_socket_global_init,
    .global_destroy = &bsd_socket_global_destroy,

    .create = &bsd_socket_create,
    .close = &bsd_socket_close,

    .set_option = &bsd_socket_set_option,

    .bind = &bsd_socket_bind,
    .connect = &bsd_socket_connect,

    .send = &bsd_socket_send,
    .sendto = &bsd_socket_sendto,
    .recv = &bsd_socket_recv,
    .recvfrom = &bsd_socket_recvfrom,

    .dump_state = &bsd_socket_dump_state,
};
