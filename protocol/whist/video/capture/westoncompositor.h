#ifndef PROTOCOL_WHIST_VIDEO_CAPTURE_WESTONCOMPOSITOR_H_
#define PROTOCOL_WHIST_VIDEO_CAPTURE_WESTONCOMPOSITOR_H_

#include <stdbool.h>
#include <wayland-client.h>
#include <wayland-server.h>
#include <libweston/libweston.h>

#include "westoncapture.h"


/** @brief */
typedef struct {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_shm *shm;
	struct whiston *whiston_root;
	struct wl_list output_list;
	bool shotInProgress;
	int current_index;
	int shot_index;
} CompositorClient;

/** @brief */
typedef struct {
	CompositorClient *compositorClient;
	struct wl_output *output;
	struct wl_buffer *buffer[2];
	int width, height, offset_x, offset_y;
	void *data[2];
	struct wl_list link;
} WhistonOutput;





typedef struct {
	struct weston_surface *wsurface;
	struct wl_listener fsv_destroy;
} WhistonFullScreenView;

typedef struct {
	struct wl_list link; /* wet_compositor.outputs */
	struct weston_output *woutput;
	WhistonFullScreenView background;
} WhistonCompOutput;


typedef struct {
	struct wl_global *global;
	struct wl_client *allowed_client;
	struct wl_listener destroy_listener;
} WhistRootObject;


typedef struct {
	struct weston_compositor *compositor;
	struct weston_output *output;
	struct weston_config *config;
	struct wet_output_config *parsed_options;
	bool drm_use_current_mode;
	struct wl_listener heads_changed_listener;
	int (*simple_output_configure)(struct weston_compositor *compositor, struct weston_output *output);
	bool init_failed;
	struct wl_list layoutput_list;	/**< wet_layoutput::compositor_link */
	struct wl_list child_process_list;
	pid_t autolaunch_pid;
	bool autolaunch_watch;
	bool use_color_manager;
	WhistRootObject whiston_root;

	struct wl_list pending_surfaces;

	struct weston_layer background_layer;
	struct weston_seat seat;
}  WhistonCompositor;

/**
 * @brief
 */
typedef struct {
	CaptureDeviceImpl base;

	WhistonCompositor whiston_compositor;
	CompositorClient client;

	struct weston_log_context *log_ctx;
	const char *display_name;
	struct wl_display *display;
	struct wl_event_loop *loop;
	size_t nsignals;
	struct wl_event_source *signals[10];
	struct weston_config *config;
	struct weston_output *output;
	struct weston_desktop *desktop;

	WhistThread weston_thread;
	volatile bool threadRunning;
	struct whist_root* whiston_object;

} WestonCaptureDevice;

bool whiston_compositor_init(WestonCaptureDevice *dev, const char *display);

bool whiston_object_init(WhistonCompositor *wcomp);

struct wl_display* whiston_connect_client(WhistonCompositor *wcompositor);

#endif /* PROTOCOL_WHIST_VIDEO_CAPTURE_WESTONCOMPOSITOR_H_ */
