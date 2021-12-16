#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mount.h>

void write_to_file(char *filename, const char *data, size_t size) {
    int fd = open(filename, O_WRONLY);
    write(fd, data, size);
    close(fd);
}

void unshare_thread() {
    // Move to a new mount namespace and and a new user namespace
    unshare(CLONE_NEWNS | CLONE_NEWUSER);

    // Appropriately map the new user namespace's root
    // to container root (uid 0), and similar for group ids

    // In order to do this from an unprivileged process since
    // Linux 3.19, we must first permanently disable the ability
    // to call `setgroups` in this namespace.
    write_to_file("/proc/thread-self/setgroups", "deny", 4);
    // The syntax here is "from to range_size".
    // Right now this is just the trivial mapping 0->0,
    // but eventually we will want to map child-namespace 0
    // to parent-namespace 1000 (whist) for uid and gid.
    // This is difficult because this is only permitted if
    // the process is uid/gid 1000 in the parent namespace.
    write_to_file("/proc/thread-self/uid_map", "0 0 1", 5);
    write_to_file("/proc/thread-self/gid_map", "0 0 1", 5);

    // This is a null mount that just changes the propagation type
    // of our filesystem at "/" to private. This means that mount
    // syscalls inside the filesystem in our namespace won't try
    // to propagate outside -- in particular, FUSE mounts will now
    // be nicely sandboxed.
    mount("none", "/", NULL, MS_REC | MS_PRIVATE, NULL);
}

int main() {
    // Unshare the current thread and everything that is spawned from it.
    unshare_thread();

    // Spawn bash in this environment.
    execl("/bin/bash", "/bin/bash", NULL);
    return 0;
}
