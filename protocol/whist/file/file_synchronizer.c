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
// Get transferring files - must be from this list
LinkedList* files = file_synchronizer_get_transferring_files();
file_synchronizer_read_next_file_chunk(linked_list_head(files), &our_chunk_to_send);

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

unsigned long global_file_id = 0;
static LinkedList transferring_files;
static WhistMutex file_synchrony_update_mutex;  // used to protect the global file synchrony
static bool is_initialized = false;
static FileTransferType enabled_actions = FILE_TRANSFER_DEFAULT;

/*
============================
Private Functions
============================
*/

static void reset_transferring_file(TransferringFile* current_file) {
    /*
        Reset the transferring file info of the passed in file
        This frees transferring files + their associated resources.

        Arguments:
            current_file (TransferringFile*): One of the files in our transferring files list

        NOTE: must be called with `file_synchrony_update_mutex` held
    */

    // Close attached resources and handles
    if (current_file->file_handle) {
        fclose(current_file->file_handle);
    }

    if (current_file->filename) {
        free(current_file->filename);
    }

    if (current_file->file_path) {
        free(current_file->file_path);
    }

    // Remove from our currently tracked transferring files
    linked_list_remove(&transferring_files, current_file);
    free(current_file);
}

static void confirm_user_file_upload(void) {
    /*
        Confirm that the user file upload transfer has begun by
        writing a trigger file for the kdialog proxy to user
    */

    FILE* fptr = fopen("/home/whist/.teleport/uploaded-file-confirm", "w");
    fprintf(fptr, "confirm-trigger");
    fclose(fptr);
}

static TransferringFile* get_write_file_with_global_id(int global_id) {
    /*
        Find the corresponding write file by iterating through our transferring files
        and finding a file global id that matches the passed in id AND is in the
        FILE_WRITE_END direction. We need this second check because there might be
        read files with conflicting indices. The unique identity of a transferring
        file across both server and client is characterized by (global_file_id, direction)

        Arguments:
            global_id (int): the global file id of the transferring file

        Returns:
            The transferring write file that matches the global id, NULL if not found

        NOTE: must be called with `file_synchrony_update_mutex` held
    */

    linked_list_for_each(&transferring_files, TransferringFile, transferring_file) {
        if (transferring_file->global_file_id == global_id &&
            transferring_file->direction == FILE_WRITE_END) {
            return transferring_file;
        }
    }
    return NULL;
}

/*
============================
Public Function Implementations
============================
*/

LinkedList* file_synchronizer_get_transferring_files(void) {
    /*
        Return a pointer to our transferring files.

        Returns:
            A pointer to a linked list of (TransferringFiles*)
    */

    if (!is_initialized) {
        LOG_ERROR("Tried to retrieve transferring files but file_synchronizer is not initialized!");
        return NULL;
    }
    return &transferring_files;
}

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

    // Initialize transferring files list
    linked_list_init(&transferring_files);

    file_synchrony_update_mutex = whist_create_mutex();
    if ((requested_actions & FILE_TRANSFER_SERVER_DROP) && init_file_drop_handler()) {
        enabled_actions = (FileTransferType)(enabled_actions | FILE_TRANSFER_SERVER_DROP);
    }
    if ((requested_actions & FILE_TRANSFER_CLIENT_DOWNLOAD)) {
        enabled_actions = (FileTransferType)(enabled_actions | FILE_TRANSFER_CLIENT_DOWNLOAD);
    }
    if ((requested_actions & FILE_TRANSFER_SERVER_UPLOAD)) {
        enabled_actions = (FileTransferType)(enabled_actions | FILE_TRANSFER_SERVER_UPLOAD);
    }

    is_initialized = true;
}

#ifdef _WIN32
#define HOME_ENV_VAR "HOMEPATH"
#else
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

    // Create a new transferring file entry and add it to our list - this is eventually freed in
    // reset_transferring_file when either the last chunk is written or the files are all reset
    TransferringFile* active_file = (TransferringFile*)malloc(sizeof(TransferringFile));
    memset(active_file, 0, sizeof(TransferringFile));
    linked_list_add_head(&transferring_files, active_file);

    LOG_INFO("Opening global file id %d for writing", file_metadata->global_file_id);

    // Set identifiers
    active_file->id = unique_id++;
    active_file->global_file_id = file_metadata->global_file_id;

    // Set transferring file filename
    active_file->filename = strdup(file_metadata->filename);

    // Get file path to which we will write
    const char* file_dir;
    switch (file_metadata->transfer_type & enabled_actions) {
        case FILE_TRANSFER_SERVER_DROP: {
            file_dir = file_drop_prepare(active_file->id, file_metadata);
            break;
        }
        case FILE_TRANSFER_SERVER_UPLOAD: {
            file_dir = "/home/whist/.teleport/uploads";
            break;
        }
        case FILE_TRANSFER_CLIENT_DOWNLOAD: {
            const char* home_dir = getenv(HOME_ENV_VAR);
            const char* downloads = "Downloads";
            char* download_file_dir = (char*)malloc(strlen(home_dir) + 1 + strlen(downloads) + 1);
            snprintf(download_file_dir, strlen(home_dir) + 1 + strlen(downloads) + 1, "%s%c%s",
                     home_dir, PATH_SEPARATOR, downloads);
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
    active_file->file_path = (char*)safe_malloc(file_path_len + 1);
    snprintf(active_file->file_path, file_path_len + 1, "%s%c%s", file_dir, PATH_SEPARATOR,
             active_file->filename);

    active_file->transfer_type = file_metadata->transfer_type;
    active_file->event_info = file_metadata->event_info;

    active_file->file_handle = fopen(active_file->file_path, "wb");
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

    TransferringFile* active_file = get_write_file_with_global_id(file_chunk->global_file_id);

    if (!active_file) {
        LOG_WARNING("Write file with global file id %d not found!", file_chunk->global_file_id);
        return false;
    }

    // If the file handle isn't being written locally or not ready for writing, then return false.
    if (active_file->direction != FILE_WRITE_END || active_file->file_handle == NULL) {
        whist_unlock_mutex(file_synchrony_update_mutex);
        return false;
    }

    switch (file_chunk->chunk_type) {
        case FILE_BODY: {
            LOG_INFO("Writing chunk to global file id %d size %zu", file_chunk->global_file_id,
                     file_chunk->size);

            // For body chunks, write the data to the file
            fwrite(file_chunk->data, 1, file_chunk->size, active_file->file_handle);
            break;
        }
        case FILE_CLOSE: {
            LOG_INFO("Finished writing to global file id %d", file_chunk->global_file_id);

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
                case FILE_TRANSFER_SERVER_UPLOAD: {
                    confirm_user_file_upload();
                    break;
                }
                default: {
                    break;
                }
            }

            // For end chunks, close the file handle and reset the slot in the array
            reset_transferring_file(active_file);
            break;
        }
        default: {
            break;
        }
    }

    whist_unlock_mutex(file_synchrony_update_mutex);
    return true;
}

void file_synchronizer_set_file_reading_basic_metadata(const char* file_path,
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

    LOG_INFO("Setting file metadata for read file id %lu", global_file_id);

    // Create a new transferring file entry and add it to our list - this is eventually freed in
    // reset_transferring_file when either the last chunk is written or the files are all reset
    TransferringFile* active_file = (TransferringFile*)malloc(sizeof(TransferringFile));
    memset(active_file, 0, sizeof(TransferringFile));
    linked_list_add_head(&transferring_files, active_file);

    // Set global identifier
    active_file->global_file_id = global_file_id++;

    // Set transferring file filepath
    size_t file_path_len = strlen(file_path);
    active_file->file_path = (char*)safe_malloc(file_path_len + 1);
    memset(active_file->file_path, 0, file_path_len + 1);
    memcpy(active_file->file_path, file_path, file_path_len);

    active_file->transfer_type = transfer_type;
    if (event_info) {
        active_file->event_info = *event_info;
    } else {
        memset(&active_file->event_info, 0, sizeof(FileEventInfo));
    }

    active_file->direction = FILE_READ_END;

    // No need for a unique identifier on a read files
    active_file->id = -1;

    whist_unlock_mutex(file_synchrony_update_mutex);
}

void file_synchronizer_open_file_for_reading(TransferringFile* active_file,
                                             FileMetadata** file_metadata_ptr) {
    /*
        Open a file for reading and generate the FileMetadata

        Arguments:
            active_file (TransferringFile*): One of our transferring files
            file_metadata_ptr (FileMetadata**): pointer to pointer for filled file metadata

        Returns:
            -> `file_metadata_ptr` will be populated with the allocated file metadata

        NOTE: `*file_metadata_ptr` will need to be freed outside of this function
    */

    whist_lock_mutex(file_synchrony_update_mutex);

    // If there is no file path set for reading or this file has already been opened,
    //     then just return
    if (active_file->file_path == NULL || active_file->file_handle != NULL) {
        *file_metadata_ptr = NULL;
        whist_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    // Open read file and reset if open failed
    LOG_INFO("Opening global file id %d for reading", active_file->global_file_id);

    active_file->file_handle = fopen(active_file->file_path, "rb");
    if (active_file->file_handle == NULL) {
        LOG_ERROR("Could not open global file id %d for reading", active_file->global_file_id);
        // If file cannot be opened, then just abort the file transfer
        reset_transferring_file(active_file);
        *file_metadata_ptr = NULL;
        whist_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    // Set file metadata filename (just the basename, not the full path)
    //     We could use `basename`, but it sadly is not cross-platform.
    char* temp_file_name = active_file->file_path;
    for (char* c = active_file->file_path; *c; ++c) {
        if (*c == PATH_SEPARATOR) temp_file_name = c + 1;
    }
    size_t filename_len = strlen(temp_file_name);

    // Set all file metadata
    FileMetadata* file_metadata =
        (FileMetadata*)allocate_region(sizeof(FileData) + filename_len + 1);
    memset(file_metadata->filename, 0, filename_len + 1);
    memcpy(file_metadata->filename, temp_file_name, filename_len);

    file_metadata->global_file_id = active_file->global_file_id;
    file_metadata->filename_len = filename_len;
    file_metadata->transfer_type = active_file->transfer_type;
    file_metadata->event_info = active_file->event_info;

    fseek(active_file->file_handle, 0L, SEEK_END);
    file_metadata->file_size = ftell(active_file->file_handle);
    fseek(active_file->file_handle, 0L, SEEK_SET);

    *file_metadata_ptr = file_metadata;

    whist_unlock_mutex(file_synchrony_update_mutex);
}

void file_synchronizer_read_next_file_chunk(TransferringFile* active_file,
                                            FileData** file_chunk_ptr) {
    /*
        Read the next file chunk from an opened transferring file

        Arguments:
            active_file (TransferringFile*): One of our transferring files
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

    // If this file is not being read locally or is not ready to be read (no file handle), return
    if (active_file->direction != FILE_READ_END || active_file->file_handle == NULL) {
        *file_chunk_ptr = NULL;
        whist_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    LOG_INFO("Reading a chunk from global file id %d", active_file->global_file_id);
    FileData* file_chunk = (FileData*)allocate_region(sizeof(FileData) + CHUNK_SIZE);
    file_chunk->size = fread(file_chunk->data, 1, CHUNK_SIZE, active_file->file_handle);

    // reallocate file chunk to only use size of read chunk
    file_chunk = (FileData*)realloc_region(file_chunk, sizeof(FileData) + file_chunk->size);

    file_chunk->global_file_id = active_file->global_file_id;

    // if no more contents to be read from file, then set to final chunk, else set to body
    if (file_chunk->size == 0) {
        LOG_INFO("Finished reading from global file id %d", file_chunk->global_file_id);
        file_chunk->chunk_type = FILE_CLOSE;
        // If last chunk, then reset entry in synchrony array
        reset_transferring_file(active_file);
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

    while (linked_list_size(&transferring_files)) {
        reset_transferring_file((TransferringFile*)linked_list_head(&transferring_files));
    }
}

void file_synchronizer_cancel_user_file_upload(void) {
    /*
        Our kde proxy waits for the uploaded-file-cancel or the uploaded-file-confirm
        trigger file to show up. Creates the one for cancellation.
    */

    FILE* fptr = fopen("/home/whist/.teleport/uploaded-file-cancel", "w");
    fprintf(fptr, "cancel-trigger");
    fclose(fptr);
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

    enabled_actions = FILE_TRANSFER_DEFAULT;
    is_initialized = false;

    whist_unlock_mutex(file_synchrony_update_mutex);
    whist_destroy_mutex(file_synchrony_update_mutex);
    LOG_INFO("Finished destroying file synchronizer");
}
