#ifndef WHIST_FILE_DOWNLOAD_H
#define WHIST_FILE_DOWNLOAD_H

/**
 * @brief                          Indicate to the user that a file has finished
 *                                 downloading.
 *
 * @param file_path                The path to the file that has finished downloading.
 */
void whist_file_download_notify_finished(const char* file_path);


//TODO: Move to a separate file!

const char *whist_file_upload_get_picked_file(void);

#endif  // WHIST_FILE_DOWNLOAD_H
