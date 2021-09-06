// Unshare a thread like a boss.

#include <stddef.h>
#include <sched.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <unistd.h>
#include "unshare.h"

// This code allows us to `unshare` a particular thread or process into
// a new user and mount namespace, appropriately mapping the new namespace
// UID/GID into the old namespace. This allows us to mount FUSE filesystems
// inside Docker containers without allowing for any privilege escalation.

void write_to_file(char *filename, const char *data, size_t size) {
    int fd = open(filename, O_WRONLY);
    write(fd, data, size);
    close(fd);
}

void unshare_thread() {
    // Move to a new mount namespace with a new user
    unshare(CLONE_NEWNS | CLONE_NEWUSER);

    // Appropriately map the new user to container root (uid 0)
    // In order to do this from an unprivileged process since
    // Linux 3.19, we must first permanently disable the ability
    // to call `setgroups` in this namespace.
    write_to_file("/proc/thread-self/setgroups", "deny", 4);
    // The syntax here is "from to range_size". Here we map
    // the single uid/gid 1000 <=> 1000. This is only legal if
    // the process is uid/gid 1000 in the parent namespace.
    write_to_file("/proc/thread-self/uid_map", "1000 1000 1", 11);
    write_to_file("/proc/thread-self/gid_map", "1000 1000 1", 11);

    // This is a null mount that just changes the propagation type
    // of our filesystem at "/" to private. This means that mount
    // syscalls inside the filesystem in our namespace won't try
    // to propagate outside -- in particular, FUSE mounts will now
    // be nicely sandboxed.
    mount("none", "/", NULL, MS_REC | MS_PRIVATE, NULL);
}
