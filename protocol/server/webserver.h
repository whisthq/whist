#ifndef WEBSERVER_H
#define WEBSERVER_H

/**
 * @brief                          Queries the webserver for various parameters
 */
void update_webserver_parameters();

/**
 * @brief                          Queries the webserver to get the using_stun
 * status
 *
 * @returns                        The using_stun status
 */
bool get_using_stun();

/**
 * @brief                          Queries the webserver for the VM password
 * status
 *
 * @returns                        The password for the VM
 */
char* get_vm_password();

/**
 * @brief                          Retrieves the protocol branch this program is
 *                                 running by asking the webserver
 *
 * @returns                        The string of the branch name
 */
char* get_branch();

/**
 * @brief                          Retrieves the container ID that this program is
 *                                 running in by asking the webserver
 *
 * @returns                        The string of the container ID
 */
char* get_container_id();

#endif
