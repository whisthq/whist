// This file is the main entrypoint for the FUSE filesystem for Fractal Teleport.

// uses fuse3
#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <pthread.h>
#include <dirent.h>
#include <time.h>
#include "unshare.h"

typedef struct FileTransferContext {
    char id[NAME_MAX + 1];          // Every file transfer will have a unique ID, which we store here as a string.
    char id_path[NAME_MAX + 1];     // `/home/fractal/.teleport/drag-drop/downloads/[ID]`. We must keep this allocated for the inotify thread to work.
    bool data_ready;                // `true` if the protocol has finished dumping the file, else `false`.
    bool active;                    // `true` if the protocol has started dumping the file, else `false`.
    char filename[NAME_MAX + 1];    // The name of the file that the protocol is dumping.
    int create_wd;                  // The inotify watch descriptor keeping track of `id_path` for FS events indicating that the file was created
} FileTransferContext;

// We just die after 1024 transfers, not worth being robust to this for the time being.
#define MAX_TRANSFERS 1024
FileTransferContext transfer_status[MAX_TRANSFERS];
size_t next_transfer_idx;

/**
 * @brief           Given the id of a file transfer,
 *                  return the index of the corresponding
 *                  `FileTransferContext`.
 *
 * @param id        The id of the file transfer, as a string.
 *
 * @return          The index of the corresponding `FileTransferContext`,
 *                  or `-1` if no such file transfer exists.
 *
 * @note           This is a linear search because I am lazy.
 */
int get_file_transfer_context_idx_by_id(const char *id) {
    for (int idx = 0; idx < next_transfer_idx; ++idx) {
        if (!strcmp(transfer_status[idx].id, id)) {
            return idx;
        }
    }
    return -1;
}

/**
 * @brief           Given the inotify watch descriptor of a
 *                  file transfer, return the index of the
 *                  corresponding `FileTransferContext`.
 *
 * @param wd        The inotify watch descriptor of the file transfer.
 *
 * @return          The index of the corresponding `FileTransferContext`,
 *                  or `-1` if no such file transfer exists.
 *
 * @note           This is a linear search because I am lazy.
 */
int get_file_transfer_context_idx_by_create_wd(int create_wd) {
    for (int idx = 0; idx < next_transfer_idx; ++idx) {
        if (transfer_status[idx].create_wd == create_wd) {
            return idx;
        }
    }
    return -1;
}

/**
 * @brief           Waits on the specified `FileTransferContext` until
 *                  the file transfer is ready.
 *
 * @param idx       The index of the `FileTransferContext` to wait for.
 *
 * @note            Right now, we poll every 500 ms until `active` and
 *                  `data_ready` are true. After the TODO (teleport-fuse-1)
 *                  is solved, we can get rid of `active`. See the description
 *                  for that issue below for details. We should eventually
 *                  use a more robust mechanism (e.g. mutexes and cond vars)
 *                  for this.
 */
void wait_for_transfer_context_ready(int idx) {
    struct timespec ts = {
        .tv_sec = 0,
        .tv_nsec = 1000 * 500
    };

    printf("Polling every 500 ms until file <%s> for ID <%s> is ready\n", transfer_status[idx].filename, transfer_status[idx].id);
    while (!transfer_status[idx].active || !transfer_status[idx].data_ready) {
        // Sleep 500 ms
        nanosleep(&ts, NULL);
    }
    printf("Finished polling for file <%s> for ID <%s>\n", transfer_status[idx].filename, transfer_status[idx].id);
}

// In our filesystem, every node is either:
typedef enum NodeType {
    NODE_TYPE_NONE = 0,     // nonexistent
    NODE_TYPE_ROOT = 1,     // the root of the FUSE filesystem, `/`
    NODE_TYPE_ID   = 2,     // an ID directory, `/[ID]/`
    NODE_TYPE_FILE = 3,     // a path to a file, `/[ID]/[filename]`
} NodeType;

// Given a path for a request to our FUSE filesystem, we would like to know:
typedef struct DirectoryTreeResult {
    NodeType node_type;         // what type of node the path corresponds to
    int transfer_context_idx;   // which transfer context, if any, the path corresponds to
} DirectoryTreeResult;

/**
 * @brief           Given a path, generate a `DirectoryTreeResult` which
 *                  describes the type of node the path corresponds to.
 *
 * @param path      The path to the node in our virtual filesystem.
 *
 * @return          The generated `DirectoryTreeResult`.
 *
 * @note            This is a useful utility function which allows us to turn
 *                  many of our FUSE functions into simple switch cases based
 *                  on the data in the `DirectoryTreeResult`.
 */
DirectoryTreeResult emulate_directory_tree(const char *path) {
    DirectoryTreeResult res = {0};

    if (!strcmp(path, "/")) {
        res.node_type = NODE_TYPE_ROOT;
        return res;
    }

    char id_buf[256];
    snprintf(id_buf, sizeof(id_buf), "%s", path);

    char *p = NULL;
    for (p = id_buf + 1; *p; ++p) {
        if (*p == '/') {
            *p = 0;
            ++p;
            // p now points to rest of path after /[id]/
            break;
        }
    }

    int idx = get_file_transfer_context_idx_by_id(id_buf + 1);

    if (idx < 0) {
        res.node_type = NODE_TYPE_NONE;
        return res;
    }

    res.transfer_context_idx = idx;

    if (*p == 0) {
        res.node_type = NODE_TYPE_ID;
        return res;
    } else if (transfer_status[idx].active && !strcmp(p, transfer_status[idx].filename)) {
        res.node_type = NODE_TYPE_FILE;
        return res;
    }

    res.node_type = NODE_TYPE_NONE;
    return res;
}

// NOTE: I am not putting in docstrings for these functions because they
// follow the FUSE spec and, quite frankly, I'm not sure what many of the
// parameters even do!

// Configure the FUSE filesystem -- several parameters can be set here.
static void *teleport_fuse_init(struct fuse_conn_info *conn,
            struct fuse_config *cfg)
{
    (void) conn;
    // Once a file is read, subsequent reads can be made from a kernel-level buffer cache
    cfg->kernel_cache = 1;
    return NULL;
}

// Get node type and permissions for a given path.
static int teleport_fuse_getattr(const char *path, struct stat *stbuf,
             struct fuse_file_info *fi)
{
    (void) fi;
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    DirectoryTreeResult dt_res = emulate_directory_tree(path);
    switch (dt_res.node_type) {
        case NODE_TYPE_ROOT:
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            break;
        case NODE_TYPE_ID:
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            break;
        case NODE_TYPE_FILE:
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            // TODO (teleport-fuse-2): Should we block here to get the real file length?
            stbuf->st_size = 50;
            break;
        default:
            res = -ENOENT;
            break;
    }

    return res;
}

// For a given directory path, get a list of navigable child nodes.
static int teleport_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi,
             enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    DirectoryTreeResult dt_res = emulate_directory_tree(path);
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    switch (dt_res.node_type) {
         case NODE_TYPE_ROOT:
            for (size_t idx = 0; idx < next_transfer_idx; ++idx) {
                if (transfer_status[idx].active) {
                    struct stat st = {0};
                    st.st_mode = S_IFDIR | 0755;
                    st.st_nlink = 2;
                    filler(buf, transfer_status[idx].id, &st, 0, 0);
                }
            }
            break;
        case NODE_TYPE_ID:
            if (transfer_status[dt_res.transfer_context_idx].active) {
                struct stat st = {0};
                st.st_mode = S_IFREG | 0444;
                st.st_nlink = 1;
                // TODO (teleport-fuse-2): Should we block here to get the real file length?
                st.st_size = 50;
                filler(buf, transfer_status[dt_res.transfer_context_idx].filename, &st, 0, 0);
            }
            break;
        default:
            return -ENOENT;
    }
    return 0;
}

// For a given file path, allow for read-only opening and cache the file descriptor in a FUSE-internal cache.
static int teleport_fuse_open(const char *path, struct fuse_file_info *fi)
{
    DirectoryTreeResult dt_res = emulate_directory_tree(path);

    switch (dt_res.node_type) {
        case NODE_TYPE_FILE:
            if ((fi->flags & O_ACCMODE) != O_RDONLY) {
                return -EACCES;
            }
            char download_path[NAME_MAX + 1] = {0};
            int idx = dt_res.transfer_context_idx;
            snprintf(download_path, NAME_MAX + 1, "%s/%s", transfer_status[idx].id_path, transfer_status[idx].filename);
            wait_for_transfer_context_ready(idx);
            fi->fh = open(download_path, fi->flags);
            break;
        default:
            return -ENOENT;
    }

    return 0;
}

// For a given file path, close the underlying file descriptor when all references to the file handle are closed.
static int teleport_fuse_release(const char *path, struct fuse_file_info *fi) {
    (void) fi;
    DirectoryTreeResult dt_res = emulate_directory_tree(path);
    switch (dt_res.node_type) {
        case NODE_TYPE_FILE:
            close(fi->fh);
            break;
        default:
            return -ENOENT;
    }
}

// For a given file path, size, and offset, return read bytes.
static int teleport_fuse_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi)
{
    DirectoryTreeResult dt_res = emulate_directory_tree(path);
    switch (dt_res.node_type) {
        case NODE_TYPE_FILE:
            // TODO (teleport-fuse-3): We should technically block here like we
            // do in open to be safe.  It's probably fine as-is though, since
            // you can't read a file without opening it first,
            // but there may be an edge case here.
            return pread(fi->fh, buf, size, offset);
        default:
            return -ENOENT;
    }
}

// Package the FUSE operations to be sent to the FUSE kernel.
static const struct fuse_operations teleport_fuse_oper = {
    .init       = teleport_fuse_init,
    .getattr	= teleport_fuse_getattr,
    .readdir	= teleport_fuse_readdir,
    .release    = teleport_fuse_release,
    .open		= teleport_fuse_open,
    .read		= teleport_fuse_read,
};

#define FRACTAL_TELEPORT_DRAG_DROP_DIRECTORY "/home/fractal/.teleport/drag-drop"
#define FRACTAL_TELEPORT_FUSE_PID_FILE FRACTAL_TELEPORT_DRAG_DROP_DIRECTORY "/pid"

/**
 * @brief Perform `mkdir -p` on the given path.
 *
 * @param path The path to create.
 *
 * @param mode The permissions to set on the created directories.
 *
 */
void mkpath(char *dir, mode_t mode) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    len = snprintf(tmp, sizeof(tmp), "%s", dir);

    if (tmp[len-1] == '/') {
        tmp[len-1] = 0;
    }

    for (p = tmp+1; *p; ++p) {

        if (*p == '/') {
            *p = 0;
            mkdir(tmp, mode);
            *p = '/';
        }
    }

    mkdir(tmp, mode);
}

// inotify_event includes a variable-length, null-terminated field for filename
#define INOTIFY_EVENT_SIZE (sizeof(struct inotify_event) + NAME_MAX + 1)
// let's assume we don't get more than 50 events at a time; we'll drop anything more
#define INOTIFY_BUFFER_SIZE (50 * INOTIFY_EVENT_SIZE)
// the file descriptor which inotify uses to coordinate and perform its operations
int inotify_fd;

/**
 * @brief                   Handle a new file creation in `/downloads/[ID]`
 *                          by modifying the appropriate `FileTransferContext`.
 *
 * @param create_wd         The watch descriptor for the directory in which the
 *                          new file was created.
 *
 * @param filename          The name of the new file
 */
void inotify_handle_file_create(int create_wd, const char *filename) {
    int idx = get_file_transfer_context_idx_by_create_wd(create_wd);
    if (idx < 0) {
        printf("Error handling file create: invalid ID <%s>\n", transfer_status[idx].id);
        return;
    }

    printf("Created new file <%s> associated with ID <%s>\n", filename, transfer_status[idx].id);
    transfer_status[idx].active = true;
    strcpy(transfer_status[idx].filename, filename);
    inotify_rm_watch(inotify_fd, create_wd);
}

/**
 * @brief                   Handle a readyfile creation for `/ready/[ID]`
 *                          by modifying the appropriate `FileTransferContext`.
 *
 * @param id               The ID of the readyfile that was created and the
 *                         associated `FileTransferContext`.
 */
void inotify_handle_file_close(const char *id) {
    int idx = get_file_transfer_context_idx_by_id(id);
    if (idx < 0) {
        printf("Error handling file close: invalid ID <%s>\n", id);
        return;
    }

    printf("Closed file <%s> associated with ID <%s>\n", transfer_status[idx].filename, id);
    transfer_status[idx].data_ready = true;
}

/**
 * @brief                   Handle a new ID folder creation in `/downloads`
 *                          by writing to a new `FileTransferContext`.
 *
 * @param id               The ID of the new ID folder.
 */
void inotify_handle_new_id(const char *id) {
    // Duplicate ID, so we skip handling it!
    if (get_file_transfer_context_idx_by_id(id) >= 0) {
        printf("Duplicate ID <%s>, skipping\n", id);
    }

    printf("Handling new ID <%s>\n", id);

    size_t current_idx = next_transfer_idx++;
    if (current_idx < MAX_TRANSFERS) {
        transfer_status[current_idx].active = false;
        transfer_status[current_idx].data_ready = false;
        strcpy(transfer_status[current_idx].id, id);
        snprintf(transfer_status[current_idx].id_path, NAME_MAX + 1, FRACTAL_TELEPORT_DRAG_DROP_DIRECTORY "/downloads/%s", id);
        transfer_status[current_idx].create_wd = inotify_add_watch(inotify_fd, transfer_status[current_idx].id_path, IN_CREATE);

        // Right after creating a watch, we need to manually check for it to avoid race conditions
        DIR *dirp = opendir(transfer_status[current_idx].id_path);

        // Check for the first file child of /downloads/[ID] and handle it
        struct dirent *dp;
        while (dp = readdir(dirp)) {
            if (dp->d_type != DT_DIR) {
                inotify_handle_file_create(transfer_status[current_idx].create_wd, dp->d_name);
                break;
            }
        }
        closedir(dirp);
    } else {
        printf("Error: Too many transfers! Dropping ID <%s>\n", id);
    }
}

// Updates the global state of `transfer_status` according to folders and files written in `/home/fractal/.teleport/drag-drop/downloads`.

/**
 * @brief                  Update the global state of `transfer_status` according
 *                         to folders and files written in the directory
 *                          `/home/fractal/.teleport/drag-drop/downloads`.
 *
 * @param opaque           Unused. Simply exists to match the `pthread_create` signature.
 *
 * @return                 Unused. Simply exists to match the `pthread_create` signature.
 */
void* multithreaded_download_watcher(void *opaque) {
    inotify_fd = inotify_init();

    mkpath(FRACTAL_TELEPORT_DRAG_DROP_DIRECTORY "/downloads", 0777);
    mkpath(FRACTAL_TELEPORT_DRAG_DROP_DIRECTORY "/ready", 0777);

    // watch descriptor for /downloads/[id] directories
    int id_wd = inotify_add_watch(inotify_fd, FRACTAL_TELEPORT_DRAG_DROP_DIRECTORY "/downloads", IN_CREATE);

    // watch descriptor for /ready/[id] files
    int ready_wd = inotify_add_watch(inotify_fd, FRACTAL_TELEPORT_DRAG_DROP_DIRECTORY "/ready", IN_CREATE);

    // Right after adding a watch, we need to also manually check for it to appropriately handle any race condition.

    // TODO (teleport-fuse-1): There's still a tiny race condition here, though
    // it's non-fatal and resolves very quickly.  The issue is that if an ID is
    // created right now, and the corresponding file and readyfile are both
    // written after the "new ID" handling below but before the "readyfile"
    // handling, then we'll briefly mark a file as ready even though the
    // filename hasn't been set yet. To avoid this for now, I block until
    // download_ready and active are BOTH true.
    DIR *dirp;

    // Handle "new ID" events that may have occurred before the watch

    dirp = opendir(FRACTAL_TELEPORT_DRAG_DROP_DIRECTORY "/downloads");

    // Check for any directory children of /downloads and handle them
    struct dirent *dp;
    while (dp = readdir(dirp)) {
        if (dp->d_type == DT_DIR &&
            // ignore the . and .. folders
            strcmp(dp->d_name, ".") &&
            strcmp(dp->d_name, "..")
        ) {
            inotify_handle_new_id(dp->d_name);
        }
    }
    closedir(dirp);

    // Handle "readyfile" events that may have occurred before the watch

    dirp = opendir(FRACTAL_TELEPORT_DRAG_DROP_DIRECTORY "/ready");

    // Check for any file children of /ready and handle them
    while (dp = readdir(dirp)) {
        if (dp->d_type != DT_DIR) {
            inotify_handle_file_close(dp->d_name);
        }
    }
    closedir(dirp);

    char events_buffer[INOTIFY_BUFFER_SIZE];
    memset(transfer_status, 0, sizeof(transfer_status));
    next_transfer_idx = 0;

    while (true) {
        // blocking read call
        size_t event_bytes = read(inotify_fd, (char *)events_buffer, INOTIFY_BUFFER_SIZE);

        size_t current_bytes = 0;

        while (current_bytes < event_bytes) {
            struct inotify_event *event = (struct inotify_event *) &events_buffer[current_bytes];
            if (event->wd == id_wd) {
                // This event is a new ID folder being created in /downloads.
                if ((event->mask & IN_CREATE) && (event->mask & IN_ISDIR)) {
                    inotify_handle_new_id(event->name);
                }
            } else if (event->wd == ready_wd) {
                // This event is a readyfile for an ID being created in /ready.
                // This signifies that the corresponding file is done being written.
                if ((event->mask & IN_CREATE) && !(event->mask & IN_ISDIR)) {
                    inotify_handle_file_close(event->name);
                }
            } else {
                // This event is a watch for a file create in /downloads/[ID].
                inotify_handle_file_create(event->wd, event->name);
            }
            current_bytes += sizeof(struct inotify_event) + event->len;
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int ret;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    // Set buffering on stdout to off to make log messages actually print nicely
    setbuf(stdout, NULL);

    // We have to unshare before creating the pthread, because apparently doing things
    // in the other order is liable to cause really funky behavior.
    unshare_thread();

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, &multithreaded_download_watcher, NULL);
    pthread_detach(thread_id);

    mkpath(FRACTAL_TELEPORT_DRAG_DROP_DIRECTORY, 0777);
    FILE *pidfile = fopen(FRACTAL_TELEPORT_FUSE_PID_FILE, "wb");
    fprintf(pidfile, "%d\n", getpid());
    fclose(pidfile);

    ret = fuse_main(args.argc, args.argv, &teleport_fuse_oper, NULL);
    fuse_opt_free_args(&args);
    return ret;
}
