#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H
/**
 * Copyright 2021 Fractal Computers, Inc., dba Whist
 * @file notifications.h
 * @brief This file contains all the code for client-side processing of notifications
 *        received from the server
 **/

/*
============================
Includes
============================
*/
#include <fractal/core/fractal.h>
#include <fractal/core/fractal_notification.h>
#include <fractal/network/network.h>

/**
 * @brief                          Recieves a notification packet from the server
 *                                 and if possible displays the notification 
 * 
 * 
 * @return                         Returns -1 if packet is not of type FRACTAL_NOTIFICATION
 *                                 or 0 on success
 **/
int display_notification(FractalPacket* packet);

#endif
