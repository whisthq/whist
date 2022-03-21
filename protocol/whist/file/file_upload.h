#ifndef WHIST_FILE_UPLOAD_H
#define WHIST_FILE_UPLOAD_H

/**
 * @brief                          Use native file select dialog to allow user to
 *                                 choose a file for upload.
 *
 * @returns                        Path of chosen file, null if no file was chosen
 */
const char *whist_file_upload_get_picked_file(void);

// TODO: Add docstring
const char *whist_multi_file_upload_get_picked_file(void);

#endif  // WHIST_FILE_UPLOAD_H
