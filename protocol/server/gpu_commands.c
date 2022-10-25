/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 */

#include "gpu_commands.h"
#include <whist/core/platform.h>

#if OS_IS(OS_LINUX)

#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "state.h"
#include <whist/core/whist_frame.h>

// Humanly nobody can open new tabs so fast. So 20 should be an unbreachable number for a backlog.
#define BACKLOG_NEW_CONNECTIONS 20
// Should be same as the one defined in "gpu/command_buffer/client/cmd_buffer_helper.cc" in whistium
// codebase.
#define IPC_FILE "/home/whist/gpu_commands"

static void do_chown(const char *file_path, const char *user_name, const char *group_name) {
    uid_t uid;
    gid_t gid;
    struct passwd *pwd;
    struct group *grp;

    pwd = getpwnam(user_name);
    if (pwd == NULL) {
        LOG_ERROR("Failed to get uid");
        return;
    }
    uid = pwd->pw_uid;

    grp = getgrnam(group_name);
    if (grp == NULL) {
        LOG_ERROR("Failed to get gid");
        return;
    }
    gid = grp->gr_gid;

    if (chown(file_path, uid, gid) == -1) {
        LOG_ERROR("chown fail");
        return;
    }
}

int multithreaded_gpu_command_receiver(void *opaque) {
    WhistServerState *state = (WhistServerState *)opaque;
    void *recv_command = safe_malloc(LARGEST_GPUFRAME_SIZE);
    // create a unix domain socket,
    // unlink, if already exists
    struct stat statbuf;
    if (stat(IPC_FILE, &statbuf) == 0) {
        if (unlink(IPC_FILE) == -1) {
            LOG_ERROR("unlink failed for IPC file %s", IPC_FILE);
            return -1;
        }
    }

    int listener;

    if ((listener = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1) {
        LOG_ERROR("AF_UNIX socket creation failed");
        return -1;
    }

    struct sockaddr_un socket_address;

    memset(&socket_address, 0, sizeof(struct sockaddr_un));
    socket_address.sun_family = AF_UNIX;
    strncpy(socket_address.sun_path, IPC_FILE, sizeof(socket_address.sun_path) - 1);

    if (bind(listener, (const struct sockaddr *)&socket_address, sizeof(struct sockaddr_un)) ==
        -1) {
        close(listener);
        LOG_ERROR("Could not bind to IPC unix socket");
        return -1;
    }

    // Change the ownership so that the browser which is running as 'whist' user can connect to this
    // socket
    do_chown(IPC_FILE, "whist", "whist");

    // Mark socket for accepting incoming connections using accept
    if (listen(listener, BACKLOG_NEW_CONNECTIONS) == -1) {
        close(listener);
        LOG_ERROR("Could not listen to IPC unix socket");
        return -1;
    }

    fd_set fds, readfds;
    FD_ZERO(&fds);
    FD_SET(listener, &fds);
    int fdmax = listener;
    bool connected = false;
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    ClientLock* client_lock = client_active_lock(state->client);

    while (client_lock != NULL) {
        // Refresh the client activation lock, to let the client (re/de)activate if it's trying to
        client_active_unlock(client_lock);
        client_lock = client_active_lock(state->client);
        if (client_lock == NULL) {
            break;
        }
        readfds = fds;
        // monitor readfds for readiness for reading
        if (select(fdmax + 1, &readfds, NULL, NULL, &timeout) == -1) {
            LOG_ERROR("Select for GPU command receiver returned error");
        }

        // Some sockets are ready. Examine readfds
        for (int fd = 0; fd < (fdmax + 1); fd++) {
            if (FD_ISSET(fd, &readfds)) {  // fd is ready for reading
                if (fd == listener) {      // request for new connection
                    if (connected) continue;
                    int fd_new;
                    if ((fd_new = accept(listener, NULL, NULL)) == -1) {
                        LOG_ERROR("Could not accept to IPC unix socket");
                    }
                    LOG_INFO("Got a new GPU connection fd_new=%d", fd_new);
                    FD_SET(fd_new, &fds);
                    if (fd_new > fdmax) fdmax = fd_new;
                    connected = true;
                } else  // data from an existing connection, receive it
                {
                    ssize_t numbytes = read(fd, recv_command, LARGEST_GPUFRAME_SIZE);

                    if (numbytes == -1)
                        LOG_WARNING("Error in reading from command buffer");
                    else if (numbytes == 0) {
                        // connection closed by client
                        LOG_WARNING("Socket %d closed by client\n", fd);
                        if (close(fd) == -1) LOG_ERROR("Error in closing gpu command fd");
                        FD_CLR(fd, &fds);
                    } else {
                        static int id = 1;
                        if (state->client->is_active) {
                            send_packet(&state->client->udp_context, PACKET_GPU, recv_command,
                                        numbytes, id, false);
                            id++;
                        } else {
                            LOG_INFO(
                                "Ignoring the GPU packet as client is not active, numbytes = %ld",
                                numbytes);
                        }
                    }
                }
            }  // if (fd == ...
        }      // for
    }          // while (1)
    close(listener);
    return 0;
}  // main
#else
// Not implemented for other OSes
int multithreaded_gpu_command_receiver(void *opaque) { return -1; }
#endif
