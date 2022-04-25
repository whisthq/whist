/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file westoncapture.h
 * @brief This file contains the code to do screen capture via libweston
============================
Usage
============================

*/

/*
============================
Includes
============================
*/
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <assert.h>
#include <stdbool.h>
#include <poll.h>

#include <sys/mman.h>
#include <cairo/cairo.h>

#include <whist/utils/threads.h>
#include <whist/logging/logging.h>

#include "capture.h"
#include "westoncapture.h"

#include <wayland-client.h>
#include <libweston/libweston.h>
#include <libweston/backend-headless.h>
#include <libweston/windowed-output-api.h>
#include "whist-protocol-client-protocol.h"


#include <wayland-server.h>
#include <weston/weston.h>

#include "westoncommon.h"
#include "westoncompositor.h"



void whiston_inject_mouse_position(void *opaque, int x, int y)
{
	WestonCaptureDevice *dev = (WestonCaptureDevice *)opaque;

	whiston_mouse_pos(dev->client.whiston_root, wl_fixed_from_int(x), wl_fixed_from_int(y));
}

void whiston_inject_mouse_buttons(void *opaque, int button, bool pressed)
{
	WestonCaptureDevice *dev = (WestonCaptureDevice *)opaque;

	whiston_mouse_buttons(dev->client.whiston_root, button, pressed ? WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED);
}


static void
display_handle_geometry(void *data,
			struct wl_output *wl_output,
			int x,
			int y,
			int physical_width,
			int physical_height,
			int subpixel,
			const char *make,
			const char *model,
			int transform)
{
	WhistonOutput *output;

	output = wl_output_get_user_data(wl_output);

	if (wl_output == output->output) {
		output->offset_x = x;
		output->offset_y = y;
	}
}

static void
display_handle_mode(void *data,
		    struct wl_output *wl_output,
		    uint32_t flags,
		    int width,
		    int height,
		    int refresh)
{
	WhistonOutput *output;

	output = wl_output_get_user_data(wl_output);

	if (wl_output == output->output && (flags & WL_OUTPUT_MODE_CURRENT)) {
		output->width = width;
		output->height = height;

		output->buffer[0] = screenshot_create_shm_buffer(output->width, output->height, &output->data[0], output->compositorClient->shm);
		output->buffer[1] = screenshot_create_shm_buffer(output->width, output->height, &output->data[1], output->compositorClient->shm);
	}
}

static const struct wl_output_listener output_listener = {
	display_handle_geometry,
	display_handle_mode
};

static void handle_global(void *data, struct wl_registry *registry,
	      uint32_t name, const char *interface, uint32_t version)
{
	WhistonOutput *output;
	CompositorClient *client = data;

	if (strcmp(interface, "wl_output") == 0) {
		output = zalloc(sizeof(*output));
		output->compositorClient = client;
		output->output = wl_registry_bind(registry, name,  &wl_output_interface, 1);
		wl_list_insert(&client->output_list, &output->link);
		wl_output_add_listener(output->output, &output_listener, output);

	} else if (strcmp(interface, "wl_shm") == 0) {
		client->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);

	} else if (strcmp(interface, "whiston") == 0) {
		client->whiston_root = wl_registry_bind(registry, name, &whiston_interface, 1);
	}
}

static void
handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	/* XXX: unimplemented */
}

static const struct wl_registry_listener registry_listener = {
	handle_global,
	handle_global_remove
};

static void
screenshot_done(void *data, struct whiston *screenshooter)
{
	CompositorClient* client = (CompositorClient*)data;
	client->shotInProgress = false;
	client->current_index = client->shot_index;
}

static const struct whiston_listener screenshooter_listener = {
	screenshot_done
};


static bool compositor_client_init(CompositorClient* client, WhistonCompositor *wcompositor, const char *display) {
	wl_list_init(&client->output_list);

	client->display = whiston_connect_client(wcompositor);
	if (client->display == NULL) {
		weston_log("fatal: failed to create display '%s': %s\n", display, strerror(errno));
		return false;
	}

	client->registry = wl_display_get_registry(client->display);
	wl_registry_add_listener(client->registry, &registry_listener, client);

	wl_display_dispatch(client->display);
	wl_display_roundtrip(client->display);
	if (client->whiston_root == NULL) {
		weston_log("display doesn't support whiston\n");
		return false;
	}

	whiston_add_listener(client->whiston_root, &screenshooter_listener, client);

	return true;
}





static int weston_reconfigure(WestonCaptureDevice* device, uint32_t width, uint32_t height,
                                    uint32_t dpi)
{
	//device->frame_buffer = screenshot_create_shm_buffer(width, height, &data, shm);
	return 0;
}


static int weston_capture_screen(WestonCaptureDevice* device)
{
	CompositorClient* client = &device->client;

	if (wl_list_empty(&client->output_list))
		return 0;


	WhistonOutput* output = container_of(client->output_list.next, WhistonOutput, link);

	if (!client->shotInProgress) {
		client->shot_index = (client->shot_index + 1) % 2;
		whiston_take_shot(device->client.whiston_root, output->output, output->buffer[client->shot_index]);
		wl_display_flush(client->display);
		client->shotInProgress = true;
	}

	struct pollfd poll_set;
	poll_set.fd = wl_display_get_fd(client->display);
	poll_set.events = POLLIN;
	poll_set.revents = 0;
	int timeout = 1000 / 60;
	char *extraFrameMsg = "";

	while (client->shotInProgress) {
		int ret;
		do {
			ret = poll(&poll_set, 1, timeout);
		} while(ret < 0 && errno == EINTR);

		if (ret < 0) {
			LOG_ERROR("error polling wayland client display");
			return -1;
		}
		if (ret == 0)
			break;

		wl_display_dispatch(client->display);
	}

	if (client->shotInProgress) {
		extraFrameMsg = "(old content)";
	}

	static int frameId = 1;
	LOG_DEBUG("weston capture frame %d%s", frameId++, extraFrameMsg);

	/*
	char filepath[] = "/tmp/imageXXXX.png";
	snprintf(filepath, sizeof(filepath), "/tmp/image-%d.png", frameId);
	cairo_surface_t *surface = cairo_image_surface_create_for_data(output->data[client->current_index],
								  CAIRO_FORMAT_ARGB32,
								  output->width,
								  output->height,
								  output->width * 4);
	cairo_surface_write_to_png(surface, filepath);
*/
	return client->shotInProgress ? 0 : 1;
}

static int weston_init(WestonCaptureDevice* device, uint32_t width, uint32_t height, uint32_t dpi)
{
	CaptureDeviceInfos* infos = device->base.infos;

	infos->width = width;
	infos->height = height;
	infos->dpi = dpi;

	return 0;
}

static int weston_get_dimensions(WestonCaptureDevice* device, uint32_t* width, uint32_t* height, uint32_t* dpi)
{
	*width = device->base.infos->width;
	*height = device->base.infos->height;
	*dpi = device->base.infos->dpi;
	return 0;
}

static int
whist_transfer_screen(WestonCaptureDevice* device) {
	return 0;
}


static int
whist_capture_get_data(WestonCaptureDevice* device, void** buf, uint32_t* stride)
{
	if (!wl_list_empty(&device->client.output_list)) {
		WhistonOutput *output = container_of(device->client.output_list.next, WhistonOutput, link);

		*buf = output->data[device->client.current_index];
		*stride = output->width * 4;
	}

	return 0;
}





CaptureDeviceImpl* create_weston_capture_device(CaptureDeviceInfos* infos, const char *display)
{
	WestonCaptureDevice *ret = safe_zalloc(sizeof(*ret));
	CaptureDeviceImpl* impl = &ret->base;

	FATAL_ASSERT(whiston_compositor_init(ret, display));

	FATAL_ASSERT(compositor_client_init(&ret->client, &ret->whiston_compositor, ret->display_name));

	impl->device_type = WESTON_DEVICE;
    impl->infos = infos;
	impl->init = (CaptureDeviceInitFn)weston_init;
	impl->reconfigure = (CaptureDeviceReconfigureFn)weston_reconfigure;
	impl->get_dimensions = (CaptureDeviceGetDimensionsFn)weston_get_dimensions;
	impl->capture_get_data = (CaptureDeviceGetDataFn)whist_capture_get_data;
	impl->capture_screen = (CaptureDeviceCaptureScreenFn)weston_capture_screen;
	impl->transfer_screen = (CaptureDeviceTransferScreenFn)whist_transfer_screen;
	return impl;
}
