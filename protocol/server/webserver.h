#ifndef WEBSERVER_H
#define WEBSERVER_H

/**
 * @brief                          Queries the webserver for various parameters
 */
void update_webserver_parameters();

/**
 * @brief                          Queries the webserver to get the using_stun
 *                                 status
 *
 * @returns                        The using_stun status
 */
bool get_using_stun();

/**
 * @brief                          Retrieves the container ID that this program is
 *                                 running in by asking the webserver
 *
 * @returns                        The string of the container ID
 */
char* get_container_id();

#endif  // WEBSERVER_H
