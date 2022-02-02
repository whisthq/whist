/**
 * Copyright Whist Technologies, Inc.
 * @file file_synchronizer.c
 * @brief This file contains code to synchronizer file transfer between
 *        the client and server
============================
Usage
============================
init_file_synchronizer(true);

FileChunk received_file_chunk;

// Write this received chunk to file
file_synchronizer_write_file_chunk(&received_file_chunk);

FileChunk* our_chunk_to_send;
file_synchronizer_read_next_file_chunk(0, &our_chunk_to_send);

if (our_chunk_to_send) {
  // We have a new file chunk, this should be sent to the server
  Send(our_chunk_to_send);
} else {
  // There is no new file chunk
}

destroy_file_synchronizer();
*/

#ifdef _WIN32
#pragma warning(disable : 4996)
#define _CRT_NONSTDC_NO_WARNINGS
#endif

/*
============================
Includes
============================
*/

#include <stdio.h>

#include <whist/core/whist.h>

#include "file_synchronizer.h"
#include "file_drop.h"
#include "file_download.h"

/*
============================
Defines
============================
*/

static TransferringFile transferring_files[NUM_TRANSFERRING_FILES];
static WhistMutex file_synchrony_update_mutex;  // used to protect the global file synchrony
static bool is_initialized = false;
static FileTransferType enabled_actions = 0;

/*
============================
Private Functions
============================
*/

static void reset_transferring_file(int file_index) {
    /*
        Reset the transferring file info at index `index`

        Arguments:
            file_index (int): the index in `transferring_files` to reset

        NOTE: must be called with `file_synchrony_update_mutex` held
    */

    if (file_index >= NUM_TRANSFERRING_FILES || file_index < 0) {
        LOG_ERROR("Index %d not available in transferring files", file_index);
        return;
    }

    TransferringFile* current_file = &transferring_files[file_index];

    if (current_file->file_handle) {
        fclose(current_file->file_handle);
    }

    if (current_file->filename) {
        free(current_file->filename);
    }

    if (current_file->file_path) {
        free(current_file->file_path);
    }

    current_file->filename = NULL;
    current_file->file_path = NULL;
    current_file->file_handle = NULL;
    current_file->direction = FILE_UNUSED;
}

static bool is_file_index_valid(int file_index) {
    /*
        Check whether the file index falls in the range of
        indices for the `transferring_files` array.

        Arguments:
            file_index (int): the index in `transferring_files`

        Returns:
            (bool): whether `file_index` is a valid index

        NOTE: must be called with `file_synchrony_update_mutex` held
    */

    return file_index >= 0 && file_index < NUM_TRANSFERRING_FILES;
}

static int find_free_transferring_file_index(void) {
    /*
        Find an available transferring file array index.

        Returns:
            (int): an available index for a transferring file, -1 if none

        NOTE: must be called with `file_synchrony_update_mutex` held
    */

    for (int file_index = 0; file_index < NUM_TRANSFERRING_FILES; file_index++) {
        if (transferring_files[file_index].file_handle == NULL &&
            transferring_files[file_index].direction == FILE_UNUSED) {
            return file_index;
        }
    }

    return -1;
}

/*
============================
Public Function Implementations
============================
*/

void init_file_synchronizer(FileTransferType requested_actions) {
    /*
        Initialize the file synchronizer

        Arguments:
            requested_actions (FileTransferType): the actions to attempt to enable
    */

    if (is_initialized) {
        LOG_ERROR(
            "Tried to init_file_synchronizer, but the file synchronizer is already initialized, "
            "probably because we're attempting a reconnection.");
        return;
    }

    // Zero out all the currently transferring file infos
    memset(transferring_files, 0, sizeof(TransferringFile) * NUM_TRANSFERRING_FILES);
    file_synchrony_update_mutex = whist_create_mutex();
    if ((requested_actions & FILE_TRANSFER_SERVER_DROP) && init_file_drop_handler()) {
        enabled_actions |= FILE_TRANSFER_SERVER_DROP;
    }
    if ((requested_actions & FILE_TRANSFER_CLIENT_DOWNLOAD)) {
        enabled_actions |= FILE_TRANSFER_CLIENT_DOWNLOAD;
    }
    is_initialized = true;
}

#ifdef _WIN32
#define sep '\\'
#define HOME_ENV_VAR "HOMEPATH"
#else
#define sep '/'
#define HOME_ENV_VAR "HOME"
#endif  // _WIN32
void file_synchronizer_open_file_for_writing(FileMetadata* file_metadata) {
    /*
        Open a file for writing based on `file_metadata`

        Arguments:
            file_metadata (FileMetadata*): pointer to the file metadata
    */

    whist_lock_mutex(file_synchrony_update_mutex);

    static int unique_id = 0;

    TransferringFile* active_file = &transferring_files[file_metadata->index];
    if (active_file->file_handle) {
        LOG_WARNING("Cannot open a new file at index %d when file handle is already open",
                    file_metadata->index);

        whist_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    LOG_INFO("Opening file index %d for writing", file_metadata->index);

    active_file->id = unique_id++;

    // Set transferring file filename
    active_file->filename = strdup(file_metadata->filename);

    // Get file path to which we will write
    const char* file_dir;
    switch (file_metadata->transfer_type & enabled_actions) {
        case FILE_TRANSFER_SERVER_DROP: {
            file_dir = file_drop_prepare(active_file->id, file_metadata);
            break;
        }
        case FILE_TRANSFER_CLIENT_DOWNLOAD: {
            const char* home_dir = getenv(HOME_ENV_VAR);
            const char* downloads = "Downloads";
            char* download_file_dir = malloc(strlen(home_dir) + 1 + strlen(downloads) + 1);
            snprintf(download_file_dir, strlen(home_dir) + 1 + strlen(downloads) + 1, "%s%c%s",
                     home_dir, sep, downloads);
            file_dir = download_file_dir;
            break;
        }
        case 0: {
            LOG_WARNING("File transfer type %d is not enabled", file_metadata->transfer_type);
            // fall through to default
        }
        default: {
            file_dir = ".";
            break;
        }
    }

    // Set transferring file filepath
    const size_t file_path_len = strlen(file_dir) + 1 + strlen(active_file->filename) + 1;
    active_file->file_path = safe_malloc(file_path_len + 1);
    snprintf(active_file->file_path, file_path_len + 1, "%s%c%s", file_dir, sep,
             active_file->filename);

    active_file->transfer_type = file_metadata->transfer_type;
    active_file->event_info = file_metadata->event_info;

    active_file->file_handle = fopen(active_file->file_path, "w");
    active_file->direction = FILE_WRITE_END;

    // Start the XDND drop process on the server as soon as the file exists
    if ((active_file->transfer_type & enabled_actions) == FILE_TRANSFER_SERVER_DROP) {
        drop_file_into_active_window(active_file);
    }

    whist_unlock_mutex(file_synchrony_update_mutex);
}

bool file_synchronizer_write_file_chunk(FileData* file_chunk) {
    /*
        Depending on the chunk type, either open the file with the given
        filename, write to the open file handle, or close the file and
        clean up resources.

        Arguments:
            file_chunk (FileData*): the info for the file chunk to be written
        Returns:
            (bool): true on success, false on failure
    */

    if (!is_initialized) {
        LOG_ERROR(
            "Tried to file_synchronizer_write_file_chunk, but the file synchronizer is not "
            "initialized");
        return false;
    }

    whist_lock_mutex(file_synchrony_update_mutex);

    if (!is_file_index_valid(file_chunk->index)) {
        LOG_ERROR("Unallowed file chunk index %d", file_chunk->index);

        whist_unlock_mutex(file_synchrony_update_mutex);
        return false;
    }

    TransferringFile* active_file = &transferring_files[file_chunk->index];

    // If the file handle isn't being written locally or not ready for writing, then return false.
    if (active_file->direction != FILE_WRITE_END || active_file->file_handle == NULL) {
        whist_unlock_mutex(file_synchrony_update_mutex);
        return false;
    }

    switch (file_chunk->chunk_type) {
        case FILE_BODY: {
            LOG_INFO("Writing chunk to file index %d size %zu", file_chunk->index,
                     file_chunk->size);

            // For body chunks, write the data to the file
            fwrite(file_chunk->data, 1, file_chunk->size, active_file->file_handle);
            break;
        }
        case FILE_CLOSE: {
            LOG_INFO("Finished writing to file index %d", file_chunk->index);

            // If on the server, write the FUSE ready file when file writing is complete
            switch ((active_file->transfer_type & enabled_actions)) {
                case FILE_TRANSFER_SERVER_DROP: {
                    file_drop_mark_ready(active_file->id);
                    break;
                }
                case FILE_TRANSFER_CLIENT_DOWNLOAD: {
                    whist_file_download_notify_finished(active_file->file_path);
                    break;
                }
                default: {
                    break;
                }
            }

            // For end chunks, close the file handle and reset the slot in the array
            reset_transferring_file(file_chunk->index);
            break;
        }
        default: {
            break;
        }
    }

    whist_unlock_mutex(file_synchrony_update_mutex);
    return true;
}

void file_synchronizer_set_file_reading_basic_metadata(char* file_path,
                                                       FileTransferType transfer_type,
                                                       FileEventInfo* event_info) {
    /*
        Set the basic file information in the file synchrony array for a file
        to be read. This function must be called for a specific file before
        the file is opened and metadata is read in file_synchronizer_open_file_for_reading.

        Arguments:
            file_path (char*): the file path of the file to be read
            transfer_type (FileTransferType): the type of the file transfer
            event_info (FileEventInfo*): event-specific information that is for
                the file transfer type. For example, for FILE_TRANSFER_SERVER_DROP,
                this would consist of the x and y coordinates of the file drop.
    */

    whist_lock_mutex(file_synchrony_update_mutex);

    // Return without creating file if no free index exists
    int file_index;
    if ((file_index = find_free_transferring_file_index()) == -1) {
        LOG_WARNING("No available index for new read file");
        whist_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    LOG_INFO("Setting file metadata for read file index %d", file_index);

    TransferringFile* active_file = &transferring_files[file_index];

    // Set transferring file filepath
    size_t file_path_len = strlen(file_path);
    active_file->file_path = safe_malloc(file_path_len + 1);
    memset(active_file->file_path, 0, file_path_len + 1);
    memcpy(active_file->file_path, file_path, file_path_len);

    active_file->transfer_type = transfer_type;
    if (event_info) {
        active_file->event_info = *event_info;
        LOG_INFO("mouse: %d %d", active_file->event_info.server_drop.x,
                 active_file->event_info.server_drop.y);
    } else {
        memset(&active_file->event_info, 0, sizeof(FileEventInfo));
    }

    active_file->direction = FILE_READ_END;

    // No need for a unique identifier on a read files
    active_file->id = -1;

    whist_unlock_mutex(file_synchrony_update_mutex);
}

void file_synchronizer_open_file_for_reading(int file_index, FileMetadata** file_metadata_ptr) {
    /*
        Open a file for reading and generate the FileMetadata

        Arguments:
            file_index (int): index of file in file synchrony array
            file_metadata_ptr (FileMetadata**): pointer to pointer for filled file metadata

        Returns:
            -> `file_metadata_ptr` will be populated with the allocated file metadata

        NOTE: `*file_metadata_ptr` will need to be freed outside of this function
    */

    whist_lock_mutex(file_synchrony_update_mutex);

    if (!is_file_index_valid(file_index)) {
        LOG_ERROR("Unallowed file chunk index %d", file_index);
        *file_metadata_ptr = NULL;
        whist_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    TransferringFile* active_file = &transferring_files[file_index];

    // If there is no file path set for reading or this file has already been opened,
    //     then just return
    if (active_file->file_path == NULL || active_file->file_handle != NULL) {
        *file_metadata_ptr = NULL;
        whist_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    // Open read file and reset if open failed
    LOG_INFO("Opening file index %d for reading", file_index);
    active_file->file_handle = fopen(active_file->file_path, "r");
    if (active_file->file_handle == NULL) {
        LOG_ERROR("Could not open file index %d for reading", file_index);
        // If file cannot be opened, then just abort the file transfer
        reset_transferring_file(file_index);
        *file_metadata_ptr = NULL;
        whist_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    // Set file metadata filename (just the basename, not the full path)
    //     We could use `basename`, but it sadly is not cross-platform.
    char* temp_file_name = active_file->file_path;
    for (char* c = active_file->file_path; *c; ++c) {
        if (*c == sep) temp_file_name = c + 1;
    }
    size_t filename_len = strlen(temp_file_name);

    // Set all file metadata
    FileMetadata* file_metadata = allocate_region(sizeof(FileData) + filename_len + 1);
    memset(file_metadata->filename, 0, filename_len + 1);
    memcpy(file_metadata->filename, temp_file_name, filename_len);

    file_metadata->index = file_index;
    file_metadata->filename_len = filename_len;
    file_metadata->transfer_type = active_file->transfer_type;
    file_metadata->event_info = active_file->event_info;

    fseek(active_file->file_handle, 0L, SEEK_END);
    file_metadata->file_size = ftell(active_file->file_handle);
    fseek(active_file->file_handle, 0L, SEEK_SET);

    *file_metadata_ptr = file_metadata;

    whist_unlock_mutex(file_synchrony_update_mutex);
}

void file_synchronizer_read_next_file_chunk(int file_index, FileData** file_chunk_ptr) {
    /*
        Read the next file chunk from an opened transferring file

        Arguments:
            file_index (int): index of the file in `transferring_files`
            file_chunk_ptr (FileData**): pointer to pointer for filled file chunk

        Returns:
            -> `file_chunk_ptr` will be populated with the allocated file chunk

        NOTE: the returned FileData* will need to be freed outside of this function
    */

    if (!is_initialized) {
        LOG_ERROR(
            "Tried to file_synchronizer_read_file_chunk, but the file synchronizer is not "
            "initialized");
        *file_chunk_ptr = NULL;
    }

    whist_lock_mutex(file_synchrony_update_mutex);

    if (!is_file_index_valid(file_index)) {
        LOG_ERROR("Unallowed file chunk index %d", file_index);

        *file_chunk_ptr = NULL;
        whist_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    TransferringFile* active_file = &transferring_files[file_index];

    // If this file is not being read locally or is not ready to be read (no file handle), return
    if (active_file->direction != FILE_READ_END || active_file->file_handle == NULL) {
        *file_chunk_ptr = NULL;
        whist_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    LOG_INFO("Reading a chunk from file index %d", file_index);
    FileData* file_chunk = allocate_region(sizeof(FileData) + CHUNK_SIZE);
    file_chunk->size = fread(file_chunk->data, 1, CHUNK_SIZE, active_file->file_handle);

    // reallocate file chunk to only use size of read chunk
    file_chunk = realloc_region(file_chunk, sizeof(FileData) + file_chunk->size);

    file_chunk->index = file_index;

    // if no more contents to be read from file, then set to final chunk, else set to body
    if (file_chunk->size == 0) {
        LOG_INFO("Finished reading from file index %d", file_index);
        file_chunk->chunk_type = FILE_CLOSE;
        // If last chunk, then reset entry in synchrony array
        reset_transferring_file(file_chunk->index);
    } else {
        file_chunk->chunk_type = FILE_BODY;
    }

    *file_chunk_ptr = file_chunk;

    whist_unlock_mutex(file_synchrony_update_mutex);
}

void reset_all_transferring_files(void) {
    /*
        Reset all transferring files
    */

    for (int file_index = 0; file_index < NUM_TRANSFERRING_FILES; file_index++) {
        reset_transferring_file(file_index);
    }
}

void destroy_file_synchronizer(void) {
    /*
        Clean up the file synchronizer
    */

    if (!is_initialized) {
        LOG_ERROR(
            "Tried to destroy_file_synchronizer, but the file synchronizer is not initialized");
        return;
    }

    whist_lock_mutex(file_synchrony_update_mutex);

    reset_all_transferring_files();

    if (enabled_actions & FILE_TRANSFER_SERVER_DROP) {
        LOG_INFO("Destroying file drop handler");
        destroy_file_drop_handler();
    }

    enabled_actions = 0;
    is_initialized = false;

    whist_unlock_mutex(file_synchrony_update_mutex);
    LOG_INFO("Finished destroying file synchronizer");
}
