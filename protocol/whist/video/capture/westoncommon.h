/*
 * westoncommon.h
 *
 *  Created on: 14 mars 2022
 *      Author: david
 */

#ifndef PROTOCOL_WHIST_VIDEO_CAPTURE_WESTONCOMMON_H_
#define PROTOCOL_WHIST_VIDEO_CAPTURE_WESTONCOMMON_H_

#include <stdbool.h>
#include <sys/socket.h>

#include <wayland-client.h>
#include <libweston/libweston.h>

#ifndef container_of
#define container_of(ptr, type, member) ({				\
	const __typeof__( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})
#endif



int os_socketpair_cloexec(int domain, int type, int protocol, int *sv);

int os_create_anonymous_file(off_t size);

struct wl_buffer *
screenshot_create_shm_buffer(int width, int height, void **data_out,
			     struct wl_shm *shm);

struct weston_log_context *whiston_initialize_logs(void);

#endif /* PROTOCOL_WHIST_VIDEO_CAPTURE_WESTONCOMMON_H_ */
