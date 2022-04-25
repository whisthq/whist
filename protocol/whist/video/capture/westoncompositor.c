
#define _GNU_SOURCE

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include <linux/input.h>

#include <wayland-client.h>

#include <xkbcommon/xkbcommon.h>

#include <libweston/libweston.h>
#include <libweston/weston-log.h>
#include <libweston-desktop/libweston-desktop.h>
#include <libweston/backend-headless.h>
#include <libweston/backend-x11.h>
#include <libweston/windowed-output-api.h>

#include "whist-protocol-server-protocol.h"

#include "westoncommon.h"
#include "westoncompositor.h"


/* a bunch of functions that are exported by libweston but not in the headers */
void weston_seat_init(struct weston_seat *seat, struct weston_compositor *ec, const char *seat_name);
void weston_seat_init_pointer(struct weston_seat *seat);
int weston_seat_init_keyboard(struct weston_seat *seat, struct xkb_keymap *keymap);
void notify_motion_absolute(struct weston_seat *seat, const struct timespec *time,
		       double x, double y);
void notify_button(struct weston_seat *seat, const struct timespec *time,
	      int32_t button, enum wl_pointer_button_state state);

#if 0
struct weston_move_grab {
	struct shell_grab base;
	wl_fixed_t dx, dy;
	bool client_initiated;
};
#endif


static WhistonCompositor *
to_wet_compositor(struct weston_compositor *compositor)
{
	return weston_compositor_get_user_data(compositor);
}

struct wet_head_tracker {
	struct wl_listener head_destroy_listener;
};



static void
screenshooter_done(void *data, enum weston_screenshooter_outcome outcome)
{
	struct wl_resource *resource = data;

	switch (outcome) {
	case WESTON_SCREENSHOOTER_SUCCESS:
		whiston_send_done(resource);
		break;
	case WESTON_SCREENSHOOTER_NO_MEMORY:
		wl_resource_post_no_memory(resource);
		break;
	default:
		break;
	}
}

static void
whiston_take_shot(struct wl_client *client,
			struct wl_resource *resource,
			struct wl_resource *output_resource,
			struct wl_resource *buffer_resource)
{
	struct weston_output *output = weston_head_from_resource(output_resource)->output;
	struct weston_buffer *buffer = weston_buffer_from_resource(buffer_resource);

	if (buffer == NULL) {
		wl_resource_post_no_memory(resource);
		return;
	}

	weston_screenshooter_shoot(output, buffer, screenshooter_done, resource);
}

static void whiston_mouse_pos(struct wl_client *client,
			  struct wl_resource *resource,
			  wl_fixed_t x,
			  wl_fixed_t y)
{
	WhistonCompositor *wcompositor = wl_resource_get_user_data(resource);

	struct timespec time;
	weston_compositor_get_time(&time);
	notify_motion_absolute(&wcompositor->seat, &time,
			wl_fixed_to_double(x) * wcompositor->output->width / MOUSE_SCALING_FACTOR,
			wl_fixed_to_double(y) * wcompositor->output->height / MOUSE_SCALING_FACTOR);
}

static void whiston_mouse_buttons(struct wl_client *client,
			      struct wl_resource *resource,
			      uint32_t button,
			      uint32_t state)
{
	WhistonCompositor *wcompositor = wl_resource_get_user_data(resource);

	struct timespec time;
	weston_compositor_get_time(&time);

	notify_button(&wcompositor->seat, &time, button, state);
}

struct whiston_interface whiston_implementation = {
	whiston_take_shot,
	whiston_mouse_pos,
	whiston_mouse_buttons,
};

static void
bind_whiston(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	WhistonCompositor *wcompositor = data;
	WhistRootObject *root = &wcompositor->whiston_root;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &whiston_interface, 1, id);

	if (!root->allowed_client) {
		wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "screenshooter failed: permission denied. "\
				       "Debug protocol must be enabled");
		return;
	} else if (client != root->allowed_client) {
		wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT,
				       "screenshooter failed: permission denied.");
		return;
	}

	wl_resource_set_implementation(resource, &whiston_implementation, data, NULL);
}


static void
screenshooter_destroy(struct wl_listener *listener, void *data)
{
	WhistRootObject *shooter =
		container_of(listener, WhistRootObject, destroy_listener);

	wl_list_remove(&shooter->destroy_listener.link);

	wl_global_destroy(shooter->global);
	free(shooter);
}

bool
whiston_object_init(WhistonCompositor *wcompositor)
{
	WhistRootObject *root = &wcompositor->whiston_root;
	struct weston_compositor *ec = wcompositor->compositor;

	root->global = wl_global_create(ec->wl_display, &whiston_interface, 1, wcompositor, bind_whiston);
	root->destroy_listener.notify = screenshooter_destroy;
	wl_signal_add(&ec->destroy_signal, &root->destroy_listener);

	return true;
}

struct wl_display* whiston_connect_client(WhistonCompositor *wcompositor)
{
	WhistRootObject *root = &wcompositor->whiston_root;
	int sv[2];

	if (os_socketpair_cloexec(AF_UNIX, SOCK_STREAM, 0, sv) < 0)
		return NULL;

	root->allowed_client = wl_client_create(wcompositor->compositor->wl_display, sv[0]);
	if (!root->allowed_client)
		return NULL;

	return wl_display_connect_to_fd(sv[1]);
}




// ###################################################################################

static int
count_remaining_heads(struct weston_output *output, struct weston_head *to_go)
{
	struct weston_head *iter = NULL;
	int n = 0;

	while ((iter = weston_output_iterate_heads(output, iter))) {
		if (iter != to_go)
			n++;
	}

	return n;
}

static void
wet_head_tracker_destroy(struct wet_head_tracker *track)
{
	wl_list_remove(&track->head_destroy_listener.link);
	free(track);
}

static void
handle_head_destroy(struct wl_listener *listener, void *data)
{
	struct weston_head *head = data;
	struct weston_output *output;
	struct wet_head_tracker *track =
		container_of(listener, struct wet_head_tracker, head_destroy_listener);

	wet_head_tracker_destroy(track);

	output = weston_head_get_output(head);

	/* On shutdown path, the output might be already gone. */
	if (!output)
		return;

	if (count_remaining_heads(output, head) > 0)
		return;

	weston_output_destroy(output);
}

static struct wet_head_tracker *
wet_head_tracker_from_head(struct weston_head *head)
{
	struct wl_listener *lis;

	lis = weston_head_get_destroy_listener(head, handle_head_destroy);
	if (!lis)
		return NULL;

	return container_of(lis, struct wet_head_tracker, head_destroy_listener);
}

/* Listen for head destroy signal.
 *
 * If a head is destroyed and it was the last head on the output, we
 * destroy the associated output.
 *
 * Do not bother destroying the head trackers on shutdown, the backend will
 * destroy the heads which calls our handler to destroy the trackers.
 */
static void
wet_head_tracker_create(WhistonCompositor *compositor, struct weston_head *head)
{
	struct wet_head_tracker *track;

	track = zalloc(sizeof *track);
	if (!track)
		return;

	track->head_destroy_listener.notify = handle_head_destroy;
	weston_head_add_destroy_listener(head, &track->head_destroy_listener);
}

static void
simple_head_enable(WhistonCompositor *wet, struct weston_head *head)
{
	int ret = 0;

	wet->output = weston_compositor_create_output_with_head(wet->compositor, head);
	if (!wet->output) {
		weston_log("Could not create an output for head \"%s\".\n", weston_head_get_name(head));
		wet->init_failed = true;

		return;
	}

	if (wet->simple_output_configure)
		ret = wet->simple_output_configure(wet->compositor, wet->output);
	if (ret < 0) {
		weston_log("Cannot configure output \"%s\".\n", weston_head_get_name(head));
		weston_output_destroy(wet->output);
		wet->output = NULL;
		wet->init_failed = true;

		return;
	}

	if (weston_output_enable(wet->output) < 0) {
		weston_log("Enabling output \"%s\" failed.\n", weston_head_get_name(head));
		weston_output_destroy(wet->output);
		wet->output = NULL;
		wet->init_failed = true;

		return;
	}

	wet_head_tracker_create(wet, head);


	/* The weston_compositor will track and destroy the output on exit. */
}

static void
simple_head_disable(struct weston_head *head)
{
	struct weston_output *output;
	struct wet_head_tracker *track;

	track = wet_head_tracker_from_head(head);
	if (track)
		wet_head_tracker_destroy(track);

	output = weston_head_get_output(head);
	assert(output);
	weston_output_destroy(output);
}

static void
simple_heads_changed(struct wl_listener *listener, void *arg)
{
	struct weston_compositor *compositor = arg;
	WhistonCompositor *wet = to_wet_compositor(compositor);
	struct weston_head *head = NULL;
	bool connected;
	bool enabled;
	bool changed;
	bool non_desktop;

	while ((head = weston_compositor_iterate_heads(wet->compositor, head))) {
		connected = weston_head_is_connected(head);
		enabled = weston_head_is_enabled(head);
		changed = weston_head_is_device_changed(head);
		non_desktop = weston_head_is_non_desktop(head);

		if (connected && !enabled && !non_desktop) {
			simple_head_enable(wet, head);
		} else if (!connected && enabled) {
			simple_head_disable(head);
		} else if (enabled && changed) {
			weston_log("Detected a monitor change on head '%s', "
				   "not bothering to do anything about it.\n",
				   weston_head_get_name(head));
		}
		weston_head_reset_device_changed(head);
	}
}


static int
whist_backend_output_configure(struct weston_compositor *compositor, struct weston_output *output)
{
	weston_output_set_scale(output, 1);
	weston_output_set_transform(output, WL_OUTPUT_TRANSFORM_NORMAL);

	const struct weston_windowed_output_api *api = weston_windowed_output_get_api(compositor);
	if (api->output_set_size(output, /*1920, 980*/1024, 800) < 0) {
		weston_log("error setting size");
	}
	return 0;
}

struct weston_config_section *weston_config_get_section(struct weston_config *config, const char *section,
			  const char *key, const char *value);
int weston_config_section_get_string(struct weston_config_section *section,
				 const char *key,
				 char **value,
				 const char *default_value);
int weston_config_section_get_int(struct weston_config_section *section,
			      const char *key,
			      int32_t *value, int32_t default_value);
int weston_config_section_get_bool(struct weston_config_section *section,
			       const char *key,
			       bool *value, bool default_value);
struct weston_config *weston_config_parse(const char *name);



static int
weston_compositor_init_config(struct weston_compositor *ec,
			      struct weston_config *config)
{
	struct xkb_rule_names xkb_names;
	struct weston_config_section *s;
	int repaint_msec;
	bool color_management;
	bool cal;

	/* weston.ini [keyboard] */
	s = weston_config_get_section(config, "keyboard", NULL, NULL);
	weston_config_section_get_string(s, "keymap_rules",
					 (char **) &xkb_names.rules, NULL);
	weston_config_section_get_string(s, "keymap_model",
					 (char **) &xkb_names.model, NULL);
	weston_config_section_get_string(s, "keymap_layout",
					 (char **) &xkb_names.layout, NULL);
	weston_config_section_get_string(s, "keymap_variant",
					 (char **) &xkb_names.variant, NULL);
	weston_config_section_get_string(s, "keymap_options",
					 (char **) &xkb_names.options, NULL);

	if (weston_compositor_set_xkb_rule_names(ec, /*&xkb_names*/NULL) < 0)
		return -1;

	weston_config_section_get_int(s, "repeat-rate",
				      &ec->kb_repeat_rate, 40);
	weston_config_section_get_int(s, "repeat-delay",
				      &ec->kb_repeat_delay, 400);

	weston_config_section_get_bool(s, "vt-switching",
				       &ec->vt_switching, true);

	/* weston.ini [core] */
	s = weston_config_get_section(config, "core", NULL, NULL);
	weston_config_section_get_int(s, "repaint-window", &repaint_msec,
				      ec->repaint_msec);
	if (repaint_msec < -10 || repaint_msec > 1000) {
		weston_log("Invalid repaint_window value in config: %d\n",
			   repaint_msec);
	} else {
		ec->repaint_msec = repaint_msec;
	}
	weston_log("Output repaint window is %d ms maximum.\n",
		   ec->repaint_msec);

	weston_config_section_get_bool(s, "color-management",
				       &color_management, false);
	/*if (color_management) {
		if (weston_compositor_load_color_manager(ec) < 0)
			return -1;
		else
			compositor->use_color_manager = true;
	}*/

	/* weston.ini [libinput] */
	/*s = weston_config_get_section(config, "libinput", NULL, NULL);
	weston_config_section_get_bool(s, "touchscreen_calibrator", &cal, 0);
	if (cal)
		weston_compositor_enable_touch_calibrator(ec,
						save_touch_device_calibration);
	*/
	return 0;
}

static void
wet_set_simple_head_configurator(struct weston_compositor *compositor,
				 int (*fn)(struct weston_compositor *compositor, struct weston_output *))
{
	WhistonCompositor *wet = to_wet_compositor(compositor);

	wet->simple_output_configure = fn;

	wet->heads_changed_listener.notify = simple_heads_changed;
	weston_compositor_add_heads_changed_listener(compositor, &wet->heads_changed_listener);
}

struct whiston_surface {
	struct weston_view *view;
	struct weston_desktop_surface *desktop_surface;
	WestonCaptureDevice* capture;
	uint32_t resize_edges;
	bool grabbed;
	struct wl_list link;
};

static void
desktop_ping_timeout(struct weston_desktop_client *dclient, void *userdata)
{
	/* not supported */
}

static void
desktop_pong(struct weston_desktop_client *dclient, void *userdata)
{
	/* not supported */
}

static void
desktop_surface_added(struct weston_desktop_surface *dsurface, void *userdata)
{
#if 0
	struct ivi_output *active_output = NULL;
	const char *app_id = NULL;
#endif
	WestonCaptureDevice *capture = (WestonCaptureDevice *)userdata;
	WhistonCompositor *whiston_compositor = &capture->whiston_compositor;
	struct weston_desktop_client *dclient;
	struct wl_client *client;
	struct whiston_surface *surface;
	struct weston_output *output = NULL;

	dclient = weston_desktop_surface_get_client(dsurface);
	client = weston_desktop_client_get_client(dclient);
	surface = zalloc(sizeof *surface);
	if (!surface) {
		wl_client_post_no_memory(client);
		return;
	}

	surface->capture = capture;
	surface->view = weston_desktop_surface_create_view(dsurface);
	if (!surface->view) {
		free(surface);
		wl_client_post_no_memory(client);
		return;
	}

	surface->desktop_surface = dsurface;
	weston_desktop_surface_set_user_data(dsurface, surface);

	wl_list_init(&surface->link);

	wl_list_insert(&whiston_compositor->pending_surfaces, &surface->link);
#if 0
	surface->ivi = ivi;
	surface->role = IVI_SURFACE_ROLE_NONE;
	surface->activated_by_default = false;
	surface->advertised_on_launch = false;
	surface->checked_pending = false;


	wl_signal_init(&surface->signal_advertise_app);

	surface->listener_advertise_app.notify = desktop_advertise_app;
	wl_signal_add(&surface->signal_advertise_app,
		      &surface->listener_advertise_app);



	if (ivi->policy && ivi->policy->api.surface_create &&
	    !ivi->policy->api.surface_create(surface, ivi)) {
		wl_client_post_no_memory(client);
		return;
	}


	app_id = weston_desktop_surface_get_app_id(desktop_surface);

	if ((active_output = ivi_layout_find_with_app_id(app_id, ivi)))
		ivi_set_pending_desktop_surface_remote(active_output, app_id);

	/* reset any caps to make sure we apply the new caps */
	ivi_seat_reset_caps_sent(ivi);

	output =  get_focused_output(ivi->compositor);
	if (!output)
		output = get_default_output(ivi->compositor);

	if (output && ivi->shell_client.ready) {
		struct ivi_output *ivi_output = to_ivi_output(output);

		/* verify if by any chance this surfaces hasn't been assigned a
		 * different role before sending the maximized state */
		if (!ivi_check_pending_surface(surface)) {
			weston_log("Setting surface to initial size of surface to %dx%d\n",
					ivi_output->area.width, ivi_output->area.height);
			weston_desktop_surface_set_maximized(desktop_surface, true);
			weston_desktop_surface_set_size(desktop_surface,
					ivi_output->area.width, ivi_output->area.height);
		}
	}
	/*
	 * We delay creating "normal" desktop surfaces until later, to
	 * give the shell-client an oppurtunity to set the surface as a
	 * background/panel.
	 * Also delay the creation in order to have a valid app_id
	 * which will be used to set the proper role.
	 */
	weston_log("Added surface %p, app_id %s to pending list\n",
			surface, app_id);
	wl_list_insert(&ivi->pending_surfaces, &surface->link);
#endif
}

static void
desktop_surface_removed(struct weston_desktop_surface *dsurface, void *userdata)
{

	struct whiston_surface *surface =
		weston_desktop_surface_get_user_data(dsurface);
	struct weston_surface *wsurface =
		weston_desktop_surface_get_surface(dsurface);
#if 0
	const char *app_id = NULL;

	struct ivi_output *output = ivi_layout_get_output_from_surface(surface);

	wl_list_remove(&surface->listener_advertise_app.link);
	surface->listener_advertise_app.notify = NULL;

	app_id = weston_desktop_surface_get_app_id(desktop_surface);

	/* special corner-case, pending_surfaces which are never activated or
	 * being assigned an output might land here so just remove the surface;
	 *
	 * the DESKTOP role can happen here as well, because we can fall-back
	 * to that when we try to determine the role type. Application that
	 * do not set the app_id will be land here, when destroyed */
	if (output == NULL && (surface->role == IVI_SURFACE_ROLE_NONE ||
			       surface->role == IVI_SURFACE_ROLE_DESKTOP))
		goto skip_output_asignment;

	assert(output != NULL);

	/* resize the active surface to the original size */
	if (surface->role == IVI_SURFACE_ROLE_SPLIT_H ||
	    surface->role == IVI_SURFACE_ROLE_SPLIT_V) {
		if (output && output->active) {
			ivi_layout_desktop_resize(output->active, output->area_saved);
		}
		/* restore the area back so we can re-use it again if needed */
		output->area = output->area_saved;
	}

	/* reset the active surface as well */
	if (output && output->active && output->active == surface) {
		output->active->view->is_mapped = false;
		output->active->view->surface->is_mapped = false;

		weston_layer_entry_remove(&output->active->view->layer_link);
		output->active = NULL;
	}

	if (surface->role == IVI_SURFACE_ROLE_REMOTE &&
	    output->type == OUTPUT_REMOTE)
		ivi_destroy_waltham_destroy(surface);

	/* check if there's a last 'remote' surface and insert a black
	 * surface view if there's no background set for that output
	 */
	if ((desktop_surface_check_last_remote_surfaces(output->ivi,
		IVI_SURFACE_ROLE_REMOTE) ||
	    desktop_surface_check_last_remote_surfaces(output->ivi,
		IVI_SURFACE_ROLE_DESKTOP)) && output->type == OUTPUT_REMOTE)
		if (!output->background)
			insert_black_surface(output);


	if (weston_surface_is_mapped(wsurface)) {
		weston_desktop_surface_unlink_view(surface->view);
		weston_view_destroy(surface->view);
	}

	/* invalidate agl-shell surfaces so we can re-use them when
	 * binding again */
	if (surface->role == IVI_SURFACE_ROLE_PANEL) {
		switch (surface->panel.edge) {
		case AGL_SHELL_EDGE_TOP:
			output->top = NULL;
			break;
		case AGL_SHELL_EDGE_BOTTOM:
			output->bottom = NULL;
			break;
		case AGL_SHELL_EDGE_LEFT:
			output->left = NULL;
			break;
		case AGL_SHELL_EDGE_RIGHT:
			output->right = NULL;
			break;
		default:
			assert(!"Invalid edge detected\n");
		}
	} else if (surface->role == IVI_SURFACE_ROLE_BACKGROUND) {
		output->background = NULL;
	}

skip_output_asignment:
	weston_log("Removed surface %p, app_id %s, role %s\n", surface,
			app_id, ivi_layout_get_surface_role_name(surface));

	if (app_id && output)
		shell_advertise_app_state(output->ivi, app_id,
					  NULL, AGL_SHELL_DESKTOP_APP_STATE_DESTROYED);

#endif
	wl_list_remove(&surface->link);

	free(surface);

}

static void
whiston_layout_activate_complete(struct weston_output *woutput,
			     struct whiston_surface *surf)
{
	struct weston_surface *wsurface = weston_desktop_surface_get_surface(surf->desktop_surface);
	struct weston_view *view = surf->view;

	if (weston_view_is_mapped(view)) {
		//weston_layer_entry_remove(&view->layer_link);
		return;
	}

	weston_view_set_output(view, woutput);
	weston_view_set_position(view,
				 woutput->x + /*output->area.x*/0,
				 woutput->y + /*output->area.y*/0);

	view->is_mapped = true;
	view->surface->is_mapped = true;

	weston_layer_entry_insert(&surf->capture->whiston_compositor.background_layer.view_list, &view->layer_link);
#if 0
	if (output->active) {
		output->active->view->is_mapped = false;
		output->active->view->surface->is_mapped = false;

		weston_layer_entry_remove(&output->active->view->layer_link);
	}
	output->previous_active = output->active;
	output->active = surf;

	weston_layer_entry_insert(&ivi->normal.view_list, &view->layer_link);
#endif
	weston_view_geometry_dirty(view);
	weston_surface_damage(view->surface);

	wsurface->is_mapped = true;
	view->is_mapped = true;

#if 0
	/*
	 * the 'remote' role now makes use of this part so make sure we don't
	 * trip the enum such that we might end up with a modified output for
	 * 'remote' role
	 */
	if (surf->role == IVI_SURFACE_ROLE_DESKTOP) {
		if (surf->desktop.pending_output)
			surf->desktop.last_output = surf->desktop.pending_output;
		surf->desktop.pending_output = NULL;
	}

	weston_log("Activation completed for app_id %s, role %s, output %s\n",
			weston_desktop_surface_get_app_id(surf->desktop_surface),
			ivi_layout_get_surface_role_name(surf), output->name);
#endif
}


static void
desktop_committed(struct weston_desktop_surface *dsurface,
		  int32_t sx, int32_t sy, void *userdata)
{
	WestonCaptureDevice *capture = (WestonCaptureDevice *)userdata;
	struct whiston_surface *surface = weston_desktop_surface_get_user_data(dsurface);

#if 0
	struct ivi_compositor *ivi = userdata;
	struct ivi_policy *policy = surface->ivi->policy;

	if (policy && policy->api.surface_commited &&
	    !policy->api.surface_commited(surface, surface->ivi))
		return;

	if (ivi->shell_client.ready && !surface->checked_pending) {
		const char * app_id =	weston_desktop_surface_get_app_id(desktop_surface);
		weston_log("Checking pending surface %p, app_id %s\n", surface,
			app_id);
		wl_list_remove(&surface->link);
		wl_list_init(&surface->link);
		ivi_check_pending_desktop_surface(surface);
		surface->checked_pending = true;
	}

	if (!surface->advertised_on_launch &&
	    !wl_list_empty(&surface->ivi->desktop_clients))
		wl_signal_emit(&surface->signal_advertise_app, surface);
#endif

	weston_compositor_schedule_repaint(capture->whiston_compositor.compositor);

	whiston_layout_activate_complete(capture->whiston_compositor.output, surface);
#if 0
	switch (surface->role) {
	case IVI_SURFACE_ROLE_DESKTOP:
	case IVI_SURFACE_ROLE_REMOTE:
		ivi_layout_desktop_committed(surface);
		break;
	case IVI_SURFACE_ROLE_POPUP:
		ivi_layout_popup_committed(surface);
		break;
	case IVI_SURFACE_ROLE_FULLSCREEN:
		ivi_layout_fullscreen_committed(surface);
		break;
	case IVI_SURFACE_ROLE_SPLIT_H:
	case IVI_SURFACE_ROLE_SPLIT_V:
		ivi_layout_split_committed(surface);
		break;
	case IVI_SURFACE_ROLE_NONE:
	case IVI_SURFACE_ROLE_BACKGROUND:
	case IVI_SURFACE_ROLE_PANEL:
	default: /* fall through */
		break;
	}
#endif
}


static void
desktop_show_window_menu(struct weston_desktop_surface *dsurface,
			 struct weston_seat *seat, int32_t x, int32_t y,
			 void *userdata)
{
	/* not supported */
}

static void
desktop_set_parent(struct weston_desktop_surface *dsurface,
		   struct weston_desktop_surface *parent, void *userdata)
{
	/* not supported */
}

#if 0
static void
noop_grab_focus(struct weston_pointer_grab *grab)
{
}

static void
noop_grab_axis(struct weston_pointer_grab *grab,
	       const struct timespec *time,
	       struct weston_pointer_axis_event *event)
{
}

static void
noop_grab_axis_source(struct weston_pointer_grab *grab,
		      uint32_t source)
{
}

static void
noop_grab_frame(struct weston_pointer_grab *grab)
{
}

static void
constrain_position(struct weston_move_grab *move, int *cx, int *cy)
{
	struct whiston_surface *shsurf = move->base.shsurf;
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	struct weston_pointer *pointer = move->base.grab.pointer;
	int x, y, bottom;
	const int safety = 50;
	pixman_rectangle32_t area;
	struct weston_geometry geometry;

	x = wl_fixed_to_int(pointer->x + move->dx);
	y = wl_fixed_to_int(pointer->y + move->dy);

	if (shsurf->shell->panel_position ==
	    WESTON_DESKTOP_SHELL_PANEL_POSITION_TOP) {
		get_output_work_area(shsurf->shell, surface->output, &area);
		geometry =
			weston_desktop_surface_get_geometry(shsurf->desktop_surface);

		bottom = y + geometry.height + geometry.y;
		if (bottom - safety < area.y)
			y = area.y + safety - geometry.height
			  - geometry.y;

		if (move->client_initiated &&
		    y + geometry.y < area.y)
			y = area.y - geometry.y;
	}

	*cx = x;
	*cy = y;
}

static void
move_grab_motion(struct weston_pointer_grab *grab,
		 const struct timespec *time,
		 struct weston_pointer_motion_event *event)
{
	struct weston_move_grab *move = (struct weston_move_grab *) grab;
	struct weston_pointer *pointer = grab->pointer;
	struct shell_surface *shsurf = move->base.shsurf;
	struct weston_surface *surface;
	int cx, cy;

	weston_pointer_move(pointer, event);
	if (!shsurf)
		return;

	surface = weston_desktop_surface_get_surface(shsurf->desktop_surface);

	constrain_position(move, &cx, &cy);

	weston_view_set_position(shsurf->view, cx, cy);

	weston_compositor_schedule_repaint(surface->compositor);
}

static void
move_grab_button(struct weston_pointer_grab *grab,
		 const struct timespec *time, uint32_t button, uint32_t state_w)
{
	struct shell_grab *shell_grab = container_of(grab, struct shell_grab,
						    grab);
	struct weston_pointer *pointer = grab->pointer;
	enum wl_pointer_button_state state = state_w;

	if (pointer->button_count == 0 &&
	    state == WL_POINTER_BUTTON_STATE_RELEASED) {
		shell_grab_end(shell_grab);
		free(grab);
	}
}

static void
move_grab_cancel(struct weston_pointer_grab *grab)
{
	struct shell_grab *shell_grab =
		container_of(grab, struct shell_grab, grab);

	shell_grab_end(shell_grab);
	free(grab);
}

static const struct weston_pointer_grab_interface move_grab_interface = {
	noop_grab_focus,
	move_grab_motion,
	move_grab_button,
	noop_grab_axis,
	noop_grab_axis_source,
	noop_grab_frame,
	move_grab_cancel,
};
#endif

static int surface_move(struct whiston_surface *shsurf, struct weston_pointer *pointer,
	     bool client_initiated)
{
#if 0
	struct weston_move_grab *move;

	if (!shsurf)
		return -1;

	if (shsurf->grabbed ||
	    weston_desktop_surface_get_fullscreen(shsurf->desktop_surface) ||
	    weston_desktop_surface_get_maximized(shsurf->desktop_surface))
		return 0;

	move = malloc(sizeof *move);
	if (!move)
		return -1;

	move->dx = wl_fixed_from_double(shsurf->view->geometry.x) -
		   pointer->grab_x;
	move->dy = wl_fixed_from_double(shsurf->view->geometry.y) -
		   pointer->grab_y;
	move->client_initiated = client_initiated;

	shell_grab_start(&move->base, &move_grab_interface, shsurf,
			 pointer, WESTON_DESKTOP_SHELL_CURSOR_MOVE);
#endif
	return 0;
}


static void
desktop_move(struct weston_desktop_surface *dsurface,
	     struct weston_seat *seat, uint32_t serial, void *userdata)
{
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	struct weston_touch *touch = weston_seat_get_touch(seat);
	struct whiston_surface *shsurf =
		weston_desktop_surface_get_user_data(dsurface);
	struct weston_surface *surface =
		weston_desktop_surface_get_surface(shsurf->desktop_surface);
	struct wl_resource *resource = surface->resource;

	struct weston_surface *focus;

	if (pointer && pointer->focus && pointer->button_count > 0 && pointer->grab_serial == serial) {
		focus = weston_surface_get_main_surface(pointer->focus->surface);
		if ((focus == surface) && (surface_move(shsurf, pointer, true) < 0))
			wl_resource_post_no_memory(resource);
	} else if (touch && touch->focus && touch->grab_serial == serial) {
		focus = weston_surface_get_main_surface(touch->focus->surface);
#if 0
		if ((focus == surface) && (surface_touch_move(shsurf, touch) < 0))
			wl_resource_post_no_memory(resource);
#endif
	}
	(void)focus;
}

static int
surface_resize(struct whiston_surface *whistsurf,
	       struct weston_pointer *pointer, uint32_t edges)
{
	//struct weston_resize_grab *resize;
	const unsigned resize_topbottom =
		WL_SHELL_SURFACE_RESIZE_TOP | WL_SHELL_SURFACE_RESIZE_BOTTOM;
	const unsigned resize_leftright =
		WL_SHELL_SURFACE_RESIZE_LEFT | WL_SHELL_SURFACE_RESIZE_RIGHT;
	const unsigned resize_any = resize_topbottom | resize_leftright;
	//struct weston_geometry geometry;

	if (whistsurf->grabbed ||
	    weston_desktop_surface_get_fullscreen(whistsurf->desktop_surface) ||
	    weston_desktop_surface_get_maximized(whistsurf->desktop_surface))
		return 0;

	/* Check for invalid edge combinations. */
	if (edges == WL_SHELL_SURFACE_RESIZE_NONE || edges > resize_any ||
	    (edges & resize_topbottom) == resize_topbottom ||
	    (edges & resize_leftright) == resize_leftright)
		return 0;

	/*resize = malloc(sizeof *resize);
	if (!resize)
		return -1;

	resize->edges = edges;*/

	/*geometry = weston_desktop_surface_get_geometry(whistsurf->desktop_surface);
	resize->width = geometry.width;
	resize->height = geometry.height;*/

	whistsurf->resize_edges = edges;
	weston_desktop_surface_set_resizing(whistsurf->desktop_surface, true);
	/*TODO: shell_grab_start(&resize->base, &resize_grab_interface, whistsurf,
			 pointer, edges);*/

	return 0;
}


static void
desktop_resize(struct weston_desktop_surface *dsurface,
	       struct weston_seat *seat, uint32_t serial,
	       enum weston_desktop_surface_edge edges, void *user_data)
{
	struct weston_pointer *pointer = weston_seat_get_pointer(seat);
	struct whiston_surface *whistsurf = weston_desktop_surface_get_user_data(dsurface);
	struct weston_surface *surface = weston_desktop_surface_get_surface(whistsurf->desktop_surface);
	struct wl_resource *resource = surface->resource;
	struct weston_surface *focus;

	if (!pointer ||
	    pointer->button_count == 0 ||
	    pointer->grab_serial != serial ||
	    pointer->focus == NULL)
		return;

	focus = weston_surface_get_main_surface(pointer->focus->surface);
	if (focus != surface)
		return;

	if (surface_resize(whistsurf, pointer, edges) < 0)
			wl_resource_post_no_memory(resource);
}

static void
desktop_fullscreen_requested(struct weston_desktop_surface *dsurface,
			     bool fullscreen, struct weston_output *output,
			     void *userdata)
{
	/* not supported */
}

static void
desktop_maximized_requested(struct weston_desktop_surface *dsurface,
			    bool maximized, void *userdata)
{
	/* not supported */
}

static void
desktop_minimized_requested(struct weston_desktop_surface *dsurface,
			    void *userdata)
{
	/* not supported */
}

static void
desktop_set_xwayland_position(struct weston_desktop_surface *dsurface,
			      int32_t x, int32_t y, void *userdata)
{
	/* not supported */
}


static const struct weston_desktop_api desktop_api = {
	.struct_size = sizeof desktop_api,
	.ping_timeout = desktop_ping_timeout,
	.pong = desktop_pong,
	.surface_added = desktop_surface_added,
	.surface_removed = desktop_surface_removed,
	.committed = desktop_committed,
	.show_window_menu = desktop_show_window_menu,
	.set_parent = desktop_set_parent,
	.move = desktop_move,
	.resize = desktop_resize,
	.fullscreen_requested = desktop_fullscreen_requested,
	.maximized_requested = desktop_maximized_requested,
	.minimized_requested = desktop_minimized_requested,
	.set_xwayland_position = desktop_set_xwayland_position,
};

static int32_t weston_thread(void* opaque) {
	WestonCaptureDevice* dev = (WestonCaptureDevice*)opaque;

	dev->threadRunning = true;
	wl_display_run(dev->display);
	return 0;
}

static int
weston_create_listening_socket(struct wl_display *display, const char *socket_name)
{
	char name_candidate[16];

	if (socket_name) {
		if (wl_display_add_socket(display, socket_name)) {
			weston_log("fatal: failed to add socket: %s\n",
				   strerror(errno));
			return -1;
		}

		setenv("WAYLAND_DISPLAY", socket_name, 1);
		return 0;
	} else {
		for (int i = 1; i <= 32; i++) {
			sprintf(name_candidate, "wayland-%d", i);
			if (wl_display_add_socket(display, name_candidate) >= 0) {
				setenv("WAYLAND_DISPLAY", name_candidate, 1);
				return 0;
			}
		}
		weston_log("fatal: failed to add socket: %s\n",
			   strerror(errno));
		return -1;
	}
}



bool whiston_compositor_init(WestonCaptureDevice *ret, const char *display)
{
	const char *socket_name = display ? display : "wayland-whist";
	const char *config_file = "/home/david/dev/git/whist/weston/output/compositor/weston.ini";
	WhistonCompositor *wcompositor = &ret->whiston_compositor;
	struct weston_headless_backend_config headless_config;
	struct weston_x11_backend_config x11_config;
	bool x11 = true;

	wl_list_init(&wcompositor->pending_surfaces);

	ret->display = wl_display_create();
	FATAL_ASSERT(ret->display);

	ret->loop = wl_display_get_event_loop(ret->display);
	FATAL_ASSERT(ret->loop);

	ret->log_ctx = whiston_initialize_logs();
	FATAL_ASSERT(ret->log_ctx);

	struct weston_compositor* compositor = wcompositor->compositor = weston_compositor_create(ret->display, ret->log_ctx, &ret->whiston_compositor, NULL);
	FATAL_ASSERT(compositor);

	/*if (weston_compositor_set_xkb_rule_names(compositor, &xkbRuleNames) < 0)
			return -1;*/

	weston_layer_init(&wcompositor->background_layer, compositor);
	weston_layer_set_position(&wcompositor->background_layer, WESTON_LAYER_POSITION_BACKGROUND);

	headless_config.base.struct_version = WESTON_HEADLESS_BACKEND_CONFIG_VERSION;
	headless_config.base.struct_size = sizeof(headless_config);
	headless_config.use_gl = false;
	headless_config.use_pixman = true;

	x11_config.base.struct_version = WESTON_X11_BACKEND_CONFIG_VERSION;
	x11_config.base.struct_size = sizeof(x11_config);
	x11_config.fullscreen = false;
	x11_config.no_input = false;
	x11_config.use_pixman = true;

	ret->config = weston_config_parse(config_file);
	FATAL_ASSERT(weston_compositor_init_config(compositor, ret->config) >= 0);

	weston_seat_init(&wcompositor->seat, compositor, "whist");
	weston_seat_init_pointer(&wcompositor->seat);

	struct xkb_rule_names xkbRuleNames;
	memset(&xkbRuleNames, 0, sizeof(xkbRuleNames));
	xkbRuleNames.model = "pc105";
	xkbRuleNames.layout = "fr";
	xkbRuleNames.variant = "";
	struct xkb_keymap *keymap;

	keymap = xkb_keymap_new_from_names(wcompositor->compositor->xkb_context, &xkbRuleNames, 0);
	if (keymap)
		weston_seat_init_keyboard(&wcompositor->seat, keymap);


	wet_set_simple_head_configurator(compositor, whist_backend_output_configure);

	int retv;

	if (x11)
		retv = weston_compositor_load_backend(compositor, WESTON_BACKEND_X11, &x11_config.base);
	else
		retv = weston_compositor_load_backend(compositor, WESTON_BACKEND_HEADLESS, &headless_config.base);

	FATAL_ASSERT(retv >= 0);

	ret->desktop = weston_desktop_create(compositor, &desktop_api, ret);

	const struct weston_windowed_output_api *api;
	api = weston_windowed_output_get_api(compositor);
	FATAL_ASSERT(api);

	FATAL_ASSERT(api->create_head(compositor, "whist") >= 0);

	weston_compositor_flush_heads_changed(compositor);

	FATAL_ASSERT(weston_create_listening_socket(ret->display, socket_name) == 0);

	weston_compositor_wake(compositor);

	/*wcompositor->whiston_root = */whiston_object_init(wcompositor);

	whist_create_thread(weston_thread, "weston", ret);

	while (!ret->threadRunning)
		whist_sleep(1);

	ret->display_name = socket_name;


	return true;
}

