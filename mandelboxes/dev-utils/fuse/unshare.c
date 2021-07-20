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
    write_to_file("/proc/self/setgroups", "deny", 4);
    // The syntax here is "from to 1". Right now this is just the
    // trivial mapping 0 -> 0. Eventually we might want
    // 0 -> uid("fractal"). The 1 means "map a range of '1' IDs".
    write_to_file("/proc/self/uid_map", "0 0 1", 5);
    write_to_file("/proc/self/gid_map", "0 0 1", 5);

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
