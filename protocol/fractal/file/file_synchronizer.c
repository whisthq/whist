/**
 * Copyright 2021 Fractal Computers, Inc. dba Whist
 * @file file_synchronizer.c
 * @brief This file contains code to synchronizer file transfer between
 *        the client and server
============================
Usage
============================
*/

/*
============================
Includes
============================
*/

#include <stdio.h>
#include <libgen.h>

#include <fractal/core/fractal.h>
#include "file_synchronizer.h"

/*
============================
Defines
============================
*/

TransferringFile transferring_files[NUM_TRANSFERRING_FILES];
FractalMutex file_synchrony_update_mutex;  // used to protect the global file synchrony
static bool is_initialized = false;

/*
============================
Private Functions
============================
*/

void reset_transferring_file(int file_index) {
    /*
        Reset the transferring file info at index `index`
        Arguments:
            file_index (int): the index in `transferring_files` to reset
        NOTE: must be called with `file_synchrony_update_mutex` held
    */

    LOG_INFO("resetting file %d", file_index);

    if (file_index >= NUM_TRANSFERRING_FILES || file_index < 0) {
        LOG_ERROR("index %d not available in transferring files", file_index);
        return;
    }

    TransferringFile* current_file = &transferring_files[file_index];

    if (current_file->file_handle) {
        LOG_INFO("freeing file_handle %p", current_file->file_handle);
        fclose(current_file->file_handle);
    }

    if (current_file->filename) {
        LOG_INFO("freeing filename %s", current_file->filename);
        free(current_file->filename);
    }

    if (current_file->file_path) {
        LOG_INFO("freeing file_path %s", current_file->file_path);
        free(current_file->file_path);
    }

    current_file->filename = NULL;
    current_file->file_path = NULL;
    current_file->file_handle = NULL;
    current_file->direction = FILE_UNUSED;
}

bool is_file_index_valid(int file_index) {
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

int find_free_transferring_file_index() {
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

void init_file_synchronizer() {
    /*
        Initialized the file synchronizer
    */

    if (is_initialized) {
        LOG_ERROR(
            "Tried to init_file_synchronizer, but the file synchronizer is already initialized");
        return;
    }

    // Zero out all the currently transferring file infos
    memset(transferring_files, 0, sizeof(TransferringFile) * NUM_TRANSFERRING_FILES);

    file_synchrony_update_mutex = fractal_create_mutex();

    is_initialized = true;
}

void file_synchronizer_open_file_for_writing(char* file_directory, FileMetadata* file_metadata,
                                             int unique_id) {
    /*
        Open a file for writing based on `file_metadata`
        Arguments:
            file_directory (char*): directory in which to write the file
            file_metadata (FileMetadata*): pointer to the file metadata
            unique_id (int): a unique ID for the file being written
    */

    fractal_lock_mutex(file_synchrony_update_mutex);

    TransferringFile* active_file = &transferring_files[file_metadata->index];

    if (active_file->file_handle) {
        LOG_WARNING("Cannot open a new file at index %d when file handle is already open",
                    file_metadata->index);

        fractal_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    active_file->id = unique_id;

    // Set transferring file filename and filepath
    int filename_len = strlen(file_metadata->filename);
    active_file->filename = safe_malloc(filename_len + 1);
    memset(active_file->filename, 0, filename_len + 1);
    memcpy(active_file->filename, file_metadata->filename, filename_len);

    int file_directory_len = strlen(file_directory);
    int file_path_len = file_directory_len + filename_len + 1;
    active_file->file_path = safe_malloc(file_path_len + 1);
    memset(active_file->file_path, 0, file_path_len + 1);
    memcpy(active_file->file_path, file_directory, file_directory_len);
    char dir_split_char = '/';
    active_file->file_path[file_directory_len] = dir_split_char;
    memcpy(active_file->file_path + file_directory_len + 1, file_metadata->filename, filename_len);

    active_file->transfer_type = file_metadata->transfer_type;
    active_file->event_info = file_metadata->event_info;

    active_file->file_handle = fopen(active_file->file_path, "w");
    active_file->direction = FILE_WRITE_END;

    fractal_unlock_mutex(file_synchrony_update_mutex);
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

    fractal_lock_mutex(file_synchrony_update_mutex);

    if (!is_file_index_valid(file_chunk->index)) {
        LOG_ERROR("Unallowed file chunk index %d", file_chunk->index);

        fractal_unlock_mutex(file_synchrony_update_mutex);
        return false;
    }

    TransferringFile* active_file = &transferring_files[file_chunk->index];

    // If the file handle isn't being written locally or not ready for writing, then return false.
    if (active_file->direction != FILE_WRITE_END || active_file->file_handle == NULL) {
        fractal_unlock_mutex(file_synchrony_update_mutex);
        return false;
    }

    switch (file_chunk->chunk_type) {
        case FILE_BODY: {
            // For body chunks, write the data to the file
            fwrite(file_chunk->data, 1, file_chunk->size, active_file->file_handle);
            break;
        }
        case FILE_CLOSE: {
            // For end chunks, close the file handle and reset the slot in the array
            reset_transferring_file(file_chunk->index);
            break;
        }
        default: {
            break;
        }
    }

    fractal_unlock_mutex(file_synchrony_update_mutex);
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

    fractal_lock_mutex(file_synchrony_update_mutex);

    int file_index;
    if ((file_index = find_free_transferring_file_index()) == -1) {
        fractal_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    TransferringFile* active_file = &transferring_files[file_index];

    // Set transferring file filepath
    int file_path_len = strlen(file_path);
    active_file->file_path = safe_malloc(file_path_len + 1);
    memset(active_file->file_path, 0, file_path_len + 1);
    memcpy(active_file->file_path, file_path, file_path_len);

    active_file->transfer_type = transfer_type;
    if (event_info) {
        active_file->event_info = *event_info;
    } else {
        memset(&active_file->event_info, 0, sizeof(FileEventInfo));
    }

    active_file->direction = FILE_READ_END;

    // Irrelevant for read files
    active_file->id = -1;

    fractal_unlock_mutex(file_synchrony_update_mutex);
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

    fractal_lock_mutex(file_synchrony_update_mutex);

    if (!is_file_index_valid(file_index)) {
        LOG_ERROR("Unallowed file chunk index %d", file_index);
        *file_metadata_ptr = NULL;
        fractal_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    TransferringFile* active_file = &transferring_files[file_index];

    // If there is no file path set for reading or this file has already been opened,
    //     then just return
    if (active_file->file_path == NULL || active_file->file_handle != NULL) {
        *file_metadata_ptr = NULL;
        fractal_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    active_file->file_handle = fopen(active_file->file_path, "r");
    if (active_file->file_handle == NULL) {
        // If file cannot be opened, then just abort the file transfer
        reset_transferring_file(file_index);
        *file_metadata_ptr = NULL;
        fractal_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    // Set file metadata filename (just the basename, not the full path)
    char* temp_file_name = basename(active_file->file_path);
    int filename_len = strlen(temp_file_name);

    FileMetadata* file_metadata = allocate_region(sizeof(FileData) + filename_len + 1);
    memset(file_metadata->filename, 0, filename_len + 1);
    memcpy(file_metadata->filename, temp_file_name, filename_len);

    file_metadata->index = file_index;
    file_metadata->filename_len = filename_len;
    file_metadata->transfer_type = active_file->transfer_type;

    fseek(active_file->file_handle, 0L, SEEK_END);
    file_metadata->file_size = ftell(active_file->file_handle);
    fseek(active_file->file_handle, 0L, SEEK_SET);

    *file_metadata_ptr = file_metadata;

    fractal_unlock_mutex(file_synchrony_update_mutex);
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

    fractal_lock_mutex(file_synchrony_update_mutex);

    if (!is_file_index_valid(file_index)) {
        LOG_ERROR("Unallowed file chunk index %d", file_index);

        *file_chunk_ptr = NULL;
        fractal_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    TransferringFile* active_file = &transferring_files[file_index];

    // If this file is not being read locally or is not ready to be read (no file handle), return
    if (active_file->direction != FILE_READ_END || active_file->file_handle == NULL) {
        *file_chunk_ptr = NULL;
        fractal_unlock_mutex(file_synchrony_update_mutex);
        return;
    }

    FileData* file_chunk = allocate_region(sizeof(FileData) + CHUNK_SIZE);
    file_chunk->size = fread(file_chunk->data, 1, CHUNK_SIZE, active_file->file_handle);

    // reallocate file chunk to only use size of read chunk
    file_chunk = realloc_region(file_chunk, sizeof(FileData) + file_chunk->size);

    file_chunk->index = file_index;

    // if no more contents to be read from file, then set to final chunk, else set to body
    if (file_chunk->size == 0) {
        file_chunk->chunk_type = FILE_CLOSE;
        // If last chunk, then reset entry in synchrony array
        reset_transferring_file(file_chunk->index);
    } else {
        file_chunk->chunk_type = FILE_BODY;
    }

    *file_chunk_ptr = file_chunk;

    fractal_unlock_mutex(file_synchrony_update_mutex);
}

void destroy_file_synchronizer() {
    /*
        Cleanup the file synchronizer
    */

    if (!is_initialized) {
        LOG_ERROR(
            "Tried to destroy_file_synchronizer, but the file synchronizer is not initialized");
        return;
    }

    fractal_lock_mutex(file_synchrony_update_mutex);

    for (int file_index = 0; file_index < NUM_TRANSFERRING_FILES; file_index++) {
        reset_transferring_file(file_index);
    }

    is_initialized = false;

    fractal_unlock_mutex(file_synchrony_update_mutex);
}
