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

#endif  // WEBSERVER_H
