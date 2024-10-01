/* diowpanel */

#include <stdio.h>
#include <string.h>
#include "runcmd.h"
#include <stdbool.h>
#include "configsgen.c"
#include "create-shm.h"
#include "async-timer.c"
#include "outputtochar.h"
#include "replacenthline.h"
#include <wayland-cursor.h>
#include "getvaluefromconf.h"
#include "outputtochararray.h"
#include "mod-volume-control.c"
#include "returnlinefromfile.h"
#include <xkbcommon/xkbcommon.h>
#include "mod-brightness-control.c"
#include <wayland-client-protocol.h>
#include <librsvg-2.0/librsvg/rsvg.h>
#include "xdg-shell-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "wlr-data-control-unstable-v1.client-protocol.h"

/* COnfigs */
char *config = NULL; // Stores the path to the config file
char *configDir = NULL; // Stores the path to the config directory
char *notesTXTPath = NULL;
char *tempTextBuff = NULL; // Temporary buffer to store and display the new note
char *cacheFilePath = NULL;
char *kbdLayoutName = NULL;
char *hiddenPassword = NULL;
char *selectedWidget = NULL; // Controls which widget on the panel is being clicked on
char *clipboardBuffer = NULL;
const char *currActiveNetwork = NULL;
static int text_scroll = 0;
static int text_scroll_net = 0;
static int xdg_serial = 0;
static int xdg_serial_empty = 0;
static int parentAppId = 0;
static int netItemsCounter = 0;
static int currentSelectedLine = 0;
static int currAciveNetworkPos = -1;
static long int currVolLevel = 0;
static bool ssidsTimer = false; // Conteol when to activate the populate SSIDs function (fix crash)
static bool addNewNote = false;
static bool ssidsReady = false;
static bool wifiEnabled = false;
static bool panelEntered = false; // Notifies about mouse pointer entering the panel surface
static bool popupEntered = false; // Notifies about mouse pointer entering any popup surface
static bool panelClicked = false;
static bool showPassword = false;
static bool nmanagerOpen = false;
static bool parentEntered = false;
static bool hotspotEnabled = false;
static bool securedNetwork = false;
static bool netDialogPageDown = false;
static bool removeLineClicked = false;
static bool keyboard_indicator = false;

/* Panel Icons */
RsvgHandle *svgSound = NULL;
RsvgHandle *svgBrightness = NULL;
RsvgHandle *svgNotes = NULL;
RsvgHandle *svgNetwork = NULL;
char *volumeHigh = NULL; // Stores the path to the volume svg
char *volumeMid = NULL;
char *volumeLow = NULL;
char *volumeOff = NULL;
char *brightnessIcon = NULL;
char *notesIcon = NULL;
char *networkIcon = NULL;
char *networkIconOff = NULL;

enum pointer_event_mask {
	POINTER_EVENT_ENTER = 1 << 0,
	POINTER_EVENT_LEAVE = 1 << 1,
	POINTER_EVENT_MOTION = 1 << 2,
	POINTER_EVENT_BUTTON = 1 << 3,
	POINTER_EVENT_AXIS = 1 << 4,
	POINTER_EVENT_AXIS_STOP = 1 << 6,
	POINTER_EVENT_AXIS_DISCRETE = 1 << 7,
};

struct pointer_event {
	uint32_t event_mask;
	wl_fixed_t surface_x;
	wl_fixed_t surface_y;
	uint32_t button;
	uint32_t state;
	uint32_t time;
	uint32_t serial;
	struct {
		bool valid;
		wl_fixed_t value;
		int32_t discrete;
	} axes[2];
};

// main state struct
struct client_state {
	/* Globals */
	struct wl_shm *wl_shm;
	struct wl_seat *wl_seat;
	struct wl_display *wl_display;
    struct xdg_wm_base *xdg_wm_base;
	struct wl_registry *wl_registry;
	struct wl_compositor *wl_compositor;
	struct zwlr_layer_shell_v1 *layer_shell;
	struct zwlr_data_control_device_v1 *zwlr_data_control_device_v1;
	struct zwlr_data_control_manager_v1 *zwlr_data_control_manager_v1;
	struct zwlr_data_control_offer_v1 *zwlr_data_control_offer_v1;
	struct zwlr_data_control_source_v1 *zwlr_data_control_source_v1;
	/* Objects */
	struct wl_cursor *wl_cursor;
	struct wl_pointer *wl_pointer;
	struct wl_surface *wl_surface;
	struct wl_keyboard *wl_keyboard;
	struct wl_surface *wl_surface_popup;
	struct wl_surface *wl_surface_empty;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct xdg_surface *xdg_surface_empty;
    struct xdg_toplevel *xdg_toplevel_empty;
	struct wl_buffer *wl_cursor_buffer;
	struct wl_surface *wl_cursor_surface;
	struct wl_cursor_theme *wl_cursor_theme;
	struct wl_cursor_image *wl_cursor_image;
	struct zwlr_layer_surface_v1 *layer_surface;
	/* State */
	struct pointer_event pointer_event;
	struct xkb_state *xkb_state;
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	bool closed;
	int32_t width;
	int32_t height;
	int32_t x_motion;
	int32_t y_motion;
	int32_t width_popup;
	int32_t height_popup;
	int32_t x_popup;
	int32_t y_popup;
	uint32_t xkb_group;
};

/* Network SSID and security */
struct network {
	char *ssids[256]; /// maximum number of networks
	char *keys[256]; /// storing whether the network if secured or free
};

/* Pointer to main client_state */
struct client_state *state_ptr = NULL;

/* Pointer to network */
struct network this_network;
struct network *network_ptr = &this_network;

/* dummy function */
static void noop() {}

/* Global funtions */
static void show_networks();
static struct wl_callback *pointer_callback = NULL; // memory leaks if used inside the function
static const struct wl_registry_listener wl_registry_listener;
static const struct xdg_surface_listener xdg_surface_listener;
static const struct wl_keyboard_listener wl_keyboard_listener;
static const struct wl_callback_listener wl_surface_frame_listener;
static const struct xdg_surface_listener xdg_surface_listener_empty;
static struct wl_buffer *draw_frame_notes(struct client_state *state);
static struct wl_buffer *draw_frame_empty(struct client_state *state);
static struct wl_buffer *draw_frame_network(struct client_state *state);
static struct wl_buffer *draw_frame_password(struct client_state *state);
static const struct zwlr_layer_surface_v1_listener layer_surface_listener;
struct zwlr_data_control_offer_v1_listener zwlr_data_control_offer_v1_listener;
struct zwlr_data_control_source_v1_listener zwlr_data_control_source_v1_listener;
static struct wl_buffer *draw_frame_volume_bright_popup(struct client_state *state);
static void zwlr_layer_surface_close(void *data, struct zwlr_layer_surface_v1 *surface);

/* get pointer events */
static void wl_pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
						struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
	(void)surface;
	(void)wl_pointer;

	struct client_state *state = data;

	/// disable evens for panel button and make it only work on the child (toplevel)
	int popupAppId = wl_proxy_get_id((struct wl_proxy *)surface);
	if (parentAppId == 0) {
		parentAppId = wl_proxy_get_id((struct wl_proxy *)surface);
	}

	if (popupAppId > parentAppId) {
		///printf("Entered Popup\n");
		popupEntered = true;
	}
	else {
		///printf("Entered parent\n");
		parentEntered = true;
	}

	state->pointer_event.event_mask |= POINTER_EVENT_ENTER;
	state->pointer_event.serial = serial;
	state->pointer_event.surface_x = surface_x,
	state->pointer_event.surface_y = surface_y;
	/// Set our pointer               
	wl_pointer_set_cursor(wl_pointer,
						  state->pointer_event.serial,
						  state->wl_cursor_surface,
						  state->wl_cursor_image->hotspot_x,
						  state->wl_cursor_image->hotspot_y);
}

static void wl_pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
																struct wl_surface *surface) {
	(void)surface;
	(void)wl_pointer;
	popupEntered = false;
	parentEntered = false;
	struct client_state *client_state = data;
	client_state->pointer_event.serial = serial;
	client_state->pointer_event.event_mask |= POINTER_EVENT_LEAVE;
}

static void wl_pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time,
												wl_fixed_t surface_x, wl_fixed_t surface_y) {
	(void)wl_pointer;
	struct client_state *client_state = data;
	client_state->pointer_event.event_mask |= POINTER_EVENT_MOTION;
	client_state->pointer_event.time = time;
	client_state->pointer_event.surface_x = surface_x,
	client_state->pointer_event.surface_y = surface_y;
}

static void wl_pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial,
               								uint32_t time, uint32_t button, uint32_t state) {
	(void)wl_pointer;
	struct client_state *client_state = data;
	client_state->pointer_event.event_mask |= POINTER_EVENT_BUTTON;
	client_state->pointer_event.time = time;
	client_state->pointer_event.serial = serial;
	client_state->pointer_event.button = button,
	client_state->pointer_event.state = state;
}

static void wl_pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time,
               											uint32_t axis, wl_fixed_t value) {
    (void)wl_pointer;
    struct client_state *client_state = data;
    client_state->pointer_event.event_mask |= POINTER_EVENT_AXIS;
    client_state->pointer_event.time = time;
    client_state->pointer_event.axes[axis].valid = true;
    client_state->pointer_event.axes[axis].value = value;
}

static void wl_pointer_axis_stop(void *data, struct wl_pointer *wl_pointer,
               									uint32_t time, uint32_t axis) {
    (void)wl_pointer;
    struct client_state *client_state = data;
    client_state->pointer_event.time = time;
    client_state->pointer_event.event_mask |= POINTER_EVENT_AXIS_STOP;
    client_state->pointer_event.axes[axis].valid = true;
}

static void wl_pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer,
													uint32_t axis, int32_t discrete) {
	(void)wl_pointer;
	struct client_state *client_state = data;
	client_state->pointer_event.event_mask |= POINTER_EVENT_AXIS_DISCRETE;
	client_state->pointer_event.axes[axis].valid = true;
	client_state->pointer_event.axes[axis].discrete = discrete;
}

static void initial_popup_open(struct client_state *state) {
	wl_surface_commit(state->wl_surface_popup);
}

static void close_popup(struct client_state *state) {
	text_scroll_net = 0;
	netItemsCounter = 0;
	addNewNote = false;
	ssidsTimer = false;
	ssidsReady = false;
	nmanagerOpen = false;
	showPassword = false;
	panelClicked = false;
	securedNetwork = false;
	removeLineClicked = false;
	/// clear SSIDs and security status names
	if (network_ptr->ssids[0] != NULL) {
		for (int i = 0; network_ptr->ssids[i] != NULL; i++) {
			free(network_ptr->ssids[i]);
			network_ptr->ssids[i] = NULL;
			free(network_ptr->keys[i]);
			network_ptr->keys[i] = NULL;
		}
	}
	if (currActiveNetwork != NULL) {
		free((void *)currActiveNetwork);
		currActiveNetwork = NULL;
	}
	/// clear temporary notes text buffer
	if (tempTextBuff != NULL) {
		free(tempTextBuff);
		tempTextBuff = NULL;
	}
	if (hiddenPassword != NULL) {
		free(hiddenPassword);
		hiddenPassword = NULL;
	}
	/// clear clipboard if there is anythung
	if (clipboardBuffer) {
		free(clipboardBuffer);
		clipboardBuffer = NULL;
	}
	if (state->xdg_toplevel) {
		xdg_toplevel_destroy(state->xdg_toplevel);
		state->xdg_toplevel = NULL;
	}
	if (state->xdg_surface) {
		xdg_surface_destroy(state->xdg_surface);
		state->xdg_surface = NULL;
	}
	if (state->xdg_toplevel_empty) {
		xdg_toplevel_destroy(state->xdg_toplevel_empty);
		state->xdg_toplevel_empty = NULL;
	}
	if (state->xdg_surface_empty) {
		xdg_surface_destroy(state->xdg_surface_empty);
		state->xdg_surface_empty = NULL;
	}
    if (state->xdg_wm_base) {
		xdg_wm_base_destroy(state->xdg_wm_base);
		state->xdg_wm_base = NULL;
	}
    if (state->wl_surface_popup) {
		wl_surface_destroy(state->wl_surface_popup);
		state->wl_surface_popup = NULL;
    }
    if (state->wl_surface_empty) {
		wl_surface_destroy(state->wl_surface_empty);
		state->wl_surface_empty = NULL;
    }
	if (state->wl_registry) {
		wl_registry_destroy(state->wl_registry);
		state->wl_registry = NULL;
	}
	///printf("------------------ Popup Closed Finished --------------------\n");
	return;
}

static void close_popup_empty(struct client_state *state) {
	if (state->xdg_toplevel_empty && state->xdg_surface_empty) {
		wl_surface_attach(state->wl_surface_empty, NULL, 0, 0);
		wl_surface_commit(state->wl_surface_empty);
		xdg_toplevel_destroy(state->xdg_toplevel_empty);
		xdg_surface_destroy(state->xdg_surface_empty);
		state->xdg_toplevel_empty = NULL;
		state->xdg_surface_empty = NULL;
		return;
	}
	return;
}

static void show_popup(struct client_state *state) {
	close_popup(state);
	if (!state->wl_registry) {
		state->wl_registry = wl_display_get_registry(state->wl_display);
		wl_registry_add_listener(state->wl_registry, &wl_registry_listener, state);
		wl_display_roundtrip(state->wl_display);
	}
	if (!state->wl_surface_popup) {
		state->wl_surface_popup = wl_compositor_create_surface(state->wl_compositor);
		wl_surface_commit(state->wl_surface_popup);
	}
	if (!state->xdg_surface) {
		state->xdg_surface = xdg_wm_base_get_xdg_surface(state->xdg_wm_base,
																	state->wl_surface_popup);
		xdg_surface_add_listener(state->xdg_surface, &xdg_surface_listener, state);
		state->xdg_toplevel = xdg_surface_get_toplevel(state->xdg_surface);
		xdg_toplevel_set_app_id(state->xdg_toplevel, "org.Diogenes.diowpanelpopup");
		xdg_surface_set_window_geometry(state->xdg_surface, state->x_popup, state->y_popup,
																	state->width_popup,
																	state->height_popup);
		wl_surface_commit(state->wl_surface_popup);
		panelClicked = true;
	}
	///printf("------------- Second time panel clicked Finished ------------\n");
	return;
}

static void show_popup_empty(struct client_state *state) {
	close_popup_empty(state);
	if (!state->wl_surface_empty) {
		state->wl_surface_empty = wl_compositor_create_surface(state->wl_compositor);
		wl_surface_commit(state->wl_surface_empty);
	}
	if (!state->xdg_wm_base) {
		state->wl_registry = wl_display_get_registry(state->wl_display);
		wl_registry_add_listener(state->wl_registry, &wl_registry_listener, state);
		wl_display_roundtrip(state->wl_display);
	}
	if (!state->xdg_surface_empty) {
		state->xdg_surface_empty = xdg_wm_base_get_xdg_surface(state->xdg_wm_base,
																	state->wl_surface_empty);
		xdg_surface_add_listener(state->xdg_surface_empty, &xdg_surface_listener_empty, state);
	}
	if (!state->xdg_toplevel_empty) {
		state->xdg_toplevel_empty = xdg_surface_get_toplevel(state->xdg_surface_empty);
		wl_surface_commit(state->wl_surface_empty);
	}
	return;
}

char *selected_widget(struct client_state *state, char *stateev) {
	/// volume open
	if (state->x_motion >= 260 && strcmp(stateev, "pressed") == 0) {
		///printf("Volume clicked\n");
		return "volume";
	}
	/// Brightness open
	else if (state->x_motion >= 220 && state->x_motion < 260 && strcmp(stateev, "pressed") == 0) {
		///printf("Brightness clicked\n");
		return "brightness";
	}
	/// Date and Time open
	else if (state->x_motion >= 140 && state->x_motion < 220 && strcmp(stateev, "pressed") == 0) {
		///printf("Datetime clicked\n");
		return "datetime";
	}
	/// keyboard layout open
	else if (state->x_motion >= 97 && state->x_motion < 140 && strcmp(stateev, "pressed") == 0) {
		///printf("Keyboard clicked\n");
		return "keyboard";
	}
	/// notes open
	else if (state->x_motion > 57 && state->x_motion < 97 && strcmp(stateev, "pressed") == 0) {
		///printf("Notes clicked\n");
		return "notes";
	}
	/// network open
	else if (state->x_motion <= 57 && strcmp(stateev, "pressed") == 0) {
		///printf("Network clicked\n");
		return "network";
	}
	else {
		return "none";
	}
}

/* Timer to close the empty buffer used for retrieveing keyboard info */
static void close_popup_empty_timer() {
	/// closing empty popup
	close_popup_empty(state_ptr);
}

/* Timer to update the popup frame after pasting clupboard content */
static void refresh_notes_frame_buffer_timer() {
	struct wl_buffer *buffer_notes = draw_frame_notes(state_ptr);
	wl_surface_attach(state_ptr->wl_surface_popup, buffer_notes, 0, 0);
	wl_surface_damage_buffer(state_ptr->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(state_ptr->wl_surface_popup);
}

/* Panel enter, leave, click actions */
static void wl_pointer_frame(void *data, struct wl_pointer *wl_pointer) {
	(void)wl_pointer;
	struct client_state *state = data;
	struct pointer_event *event = &state->pointer_event;
	char *stateev = event->state == WL_POINTER_BUTTON_STATE_RELEASED ? "released" : "pressed";

	uint32_t axis_events = POINTER_EVENT_AXIS | POINTER_EVENT_AXIS_DISCRETE | \
																	POINTER_EVENT_AXIS_STOP;
	/// changing volume with mouse wheel
	if (event->event_mask & axis_events) {
		for (size_t i = 0; i < 2; ++i) {
			if (!event->axes[i].valid) {
				continue;
			}
			/// listening to both mouse wheel and touchpad scroll events
			if ((event->event_mask & POINTER_EVENT_AXIS_DISCRETE) || \
												(event->event_mask & POINTER_EVENT_AXIS)) {
				///fprintf(stderr, "discrete %d\n", event->axes[i].discrete);
				if ((event->axes[i].discrete > 0) || (wl_fixed_to_int(event->axes[i].value) > 0)) {
					/// wheel down event
					///printf("Volume Down\n");
					change_volume_level("volume down");
				}
				else {
					/// wheel up event
					///printf("Volume Up\n");
					change_volume_level("volume up");
				}
			}
		}
	}

	if (event->event_mask & POINTER_EVENT_ENTER && !panelClicked) {
		/// autohide/show show panel
		///printf("Panel Entered\n");
		panelEntered = true;
		state->height = 40;
		zwlr_layer_surface_v1_set_size(state->layer_surface, state->width, state->height);
		pointer_callback = wl_display_sync(state->wl_display);
		wl_callback_add_listener(pointer_callback, &wl_surface_frame_listener, state);
		/// opening empty buffer first time to read keyboard info
		if (xdg_serial_empty == 0) {
			wl_surface_commit(state->wl_surface_empty);
			async_timer(close_popup_empty_timer, 3);
		}
		/// opening empty buffer second time to read keyboard info
		else {
			show_popup_empty(state);
		}
	}

	if (event->event_mask & POINTER_EVENT_LEAVE && !panelClicked) {
		/// autohide/show hide panel, prevent panel hide while popup is opened
		///printf("Panel Left\n");
		panelEntered = false;
		state->height = 1;
		zwlr_layer_surface_v1_set_size(state->layer_surface, state->width, state->height);
		/// closing empty popup
		close_popup_empty(state);
	}

	if (event->event_mask & POINTER_EVENT_BUTTON) {
		///fprintf(stderr, "button %d %s\n", event->button, stateev);
		/**************** handling pointer events, motions, clicks, hoverings ******************/
		/// close the app on right click
		if (event->button == 273) {
			state->closed = true;
			return;
		}
		/// popup toggle open/close
		if (strcmp(stateev, "pressed") == 0 && panelClicked && !popupEntered) {
			///printf("----------------------- Popup Closed ------------------------\n");
			close_popup(state);
		}
		else if (xdg_serial == 0 && strcmp(stateev, "pressed") == 0 && !panelClicked && \
																				!popupEntered) {
			///printf("----------------- First time panel clicked ------------------\n");

			/// selected widget clicked
			selectedWidget = selected_widget(state, stateev);
			///printf("selectedWidget: %s\n", selectedWidget);

			/// do nothing on keyboard layout click
			if (strcmp(selectedWidget, "keyboard") == 0) {
				panelClicked = false;
				///return;
			}
			else {
				/// opens popup the first time
				panelClicked = true;
				initial_popup_open(state);
				///return;
			}
		}
		else if (xdg_serial != 0 && strcmp(stateev, "pressed") == 0 && !panelClicked && \
																				!popupEntered) {
			///printf("---------------- Second time panel clicked ------------------\n");
			///panelClicked = true;
			/// selected widget clicked
			selectedWidget = selected_widget(state, stateev);
			///printf("selectedWidget: %s\n", selectedWidget);

			/// do nothing on keyboard layout click
			if (strcmp(selectedWidget, "keyboard") == 0) {
				panelClicked = false;
				///return;
			}
			else {
				/// open popup the second and all the next times
				show_popup(state);
				///return;
			}
		}
		/// clicking inside popup
		if (strcmp(stateev, "pressed") == 0 && popupEntered) {
			///printf("-------------------- Cliked inside popup --------------------\n");
			///int cursonr_position_y = state->y_motion;
			///printf("mouse y position: %d\n", cursonr_position_y);
			int volumeLevel = (state->y_motion - 130);
			if (state->y_motion >= 40 && state->y_motion <= 130) {
				if (volumeLevel < 0) {
					/// turn negative number into positive
					int volumeLevelNegative = abs(volumeLevel);
					volumeLevel = volumeLevelNegative;
				}
				///printf("volumeLevel: %d\n", volumeLevel);
				if (volumeLevel > get_max_volume_level()) {
					volumeLevel = get_max_volume_level();
				}
				/// setting volume level on clicking inside
				if (strcmp(selectedWidget, "volume") == 0) {
					set_volume_level(volumeLevel);
					/// refresh the frame and adjust the volume bar
					struct wl_buffer *buffer_volume = draw_frame_volume_bright_popup(state);
					wl_surface_attach(state->wl_surface_popup, buffer_volume, 0, 0);
					wl_surface_damage_buffer(state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
					wl_surface_commit(state->wl_surface_popup);
				}
				/// setting brightness level on clicking inside
				if (strcmp(selectedWidget, "brightness") == 0) {
					const char *path = get_char_value_from_conf(config, "brightness_file_path");
					/// prevent total screen brack
					if (volumeLevel <= 7) {
						volumeLevel = 7;
					}
					set_brightness(volumeLevel, path);
					free((void *)path);
					/// refresh the frame and adjust the volume bar
					struct wl_buffer *buffer_volume = draw_frame_volume_bright_popup(state);
					wl_surface_attach(state->wl_surface_popup, buffer_volume, 0, 0);
					wl_surface_damage_buffer(state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
					wl_surface_commit(state->wl_surface_popup);
				}
			}
			/// plus volumr click
			if (state->y_motion < 40) {
				/// increasing volume level on clicking plus
				if (strcmp(selectedWidget, "volume") == 0) {
					change_volume_level("volume up");
					/// refresh the frame and adjust the volume bar
					struct wl_buffer *buffer_volume = draw_frame_volume_bright_popup(state);
					wl_surface_attach(state->wl_surface_popup, buffer_volume, 0, 0);
					wl_surface_damage_buffer(state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
					wl_surface_commit(state->wl_surface_popup);
				}
				/// increasing brightness level on clicking plus
				if (strcmp(selectedWidget, "brightness") == 0) {
					const char *path = get_char_value_from_conf(config, "brightness_file_path");
					int currentBrightness = get_current_brightness(path);
					/// prevent over brightness
					if (currentBrightness >= get_max_volume_level()) {
						currentBrightness = get_max_volume_level();
						set_brightness(100, path);
						return;
					}
					if (currentBrightness >= 100) {
						set_brightness(100, path);
					}
					set_brightness((currentBrightness + 3), path);
					free((void *)path);
					/// refresh the frame and adjust the volume bar
					struct wl_buffer *buffer_volume = draw_frame_volume_bright_popup(state);
					wl_surface_attach(state->wl_surface_popup, buffer_volume, 0, 0);
					wl_surface_damage_buffer(state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
					wl_surface_commit(state->wl_surface_popup);
				}
			}
			/// minus volumr click
			if (state->y_motion > 130) {
				/// decreasing volume level
				if (strcmp(selectedWidget, "volume") == 0) {
					change_volume_level("volume down");
					/// refresh the frame and adjust the volume bar
					struct wl_buffer *buffer_volume = draw_frame_volume_bright_popup(state);
					wl_surface_attach(state->wl_surface_popup, buffer_volume, 0, 0);
					wl_surface_damage_buffer(state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
					wl_surface_commit(state->wl_surface_popup);
				}
				/// decreasing brightness level
				if (strcmp(selectedWidget, "brightness") == 0) {
					const char *path = get_char_value_from_conf(config, "brightness_file_path");
					int currentBrightness = get_current_brightness(path);
					/// prevent low brightness
					if (currentBrightness < 10) {
						currentBrightness = 10;
					}
					set_brightness((currentBrightness - 3), path);
					free((void *)path);
					/// refresh the frame and adjust the volume bar
					struct wl_buffer *buffer_volume = draw_frame_volume_bright_popup(state);
					wl_surface_attach(state->wl_surface_popup, buffer_volume, 0, 0);
					wl_surface_damage_buffer(state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
					wl_surface_commit(state->wl_surface_popup);
				}
			}
			/// clicking page down in notes
			if (strcmp(selectedWidget, "notes") == 0) {
				///printf("Clicked inside notes\n");
				if (state->y_motion >= 500 && state->y_motion < 530 && \
											state->x_motion >= 570 && state->x_motion < 600) {
					///printf("Notes page down\n");
					/// 15 lines fit in the window
					text_scroll = text_scroll + 15;
					struct wl_buffer *buffer_notes = draw_frame_notes(state);
					wl_surface_attach(state->wl_surface_popup, buffer_notes, 0, 0);
					wl_surface_damage_buffer(state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
					wl_surface_commit(state->wl_surface_popup);
					///return;
				}
				else if (state->y_motion >= 470 && state->y_motion < 500 && \
											state->x_motion >= 570 && state->x_motion < 600) {
					///printf("Notes page up\n");
					/// scrolling back
					text_scroll = text_scroll - 15;
					currentSelectedLine = currentSelectedLine - 15;
					struct wl_buffer *buffer_notes = draw_frame_notes(state);
					wl_surface_attach(state->wl_surface_popup, buffer_notes, 0, 0);
					wl_surface_damage_buffer(state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
					wl_surface_commit(state->wl_surface_popup);
					///return;
				}
				else if (state->y_motion >= 500 && state->y_motion < 530 && \
											state->x_motion >= 3 && state->x_motion < 200) {
					///printf("Add New Note\n");
					addNewNote = true;
					/// adding a new line before pasting clipboard content
					FILE *file = fopen(notesTXTPath, "a");
					fprintf(file, "\n");
					fclose(file);
					/// storing initial text into temporary buffer
					if (tempTextBuff == NULL) {
						tempTextBuff = malloc(sizeof(char) * 2048);
					}
					strncpy(tempTextBuff, "|type the new note here|", 27);
					tempTextBuff[27] = '\0';
					struct wl_buffer *buffer_notes = draw_frame_notes(state);
					wl_surface_attach(state->wl_surface_popup, buffer_notes, 0, 0);
					wl_surface_damage_buffer(state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
					wl_surface_commit(state->wl_surface_popup);
				}
				else if (state->y_motion >= 500 && state->y_motion < 530 && \
											state->x_motion >= 220 && state->x_motion < 400) {
					///printf("Paste Here\n");
					/// adding a new line before pasting clipboard content
					FILE *file = fopen(notesTXTPath, "a");
					fprintf(file, "\n");
					fclose(file);
					/// getting data from clipboard and writing to temp file
					int fd = open(notesTXTPath, O_WRONLY | O_APPEND);
					zwlr_data_control_offer_v1_receive(state->zwlr_data_control_offer_v1,
																"text/plain;charset=utf-8", fd);
					close(fd);
					async_timer(refresh_notes_frame_buffer_timer, 1);
				}
				else if (state->y_motion >= 500 && state->y_motion < 530 && \
											state->x_motion >= 400 && state->x_motion < 550) {
					///printf("Remove selected line\n");
					if (removeLineClicked) {
						removeLineClicked = false;
					}
					else {
						removeLineClicked = true;
					}
					struct wl_buffer *buffer_notes = draw_frame_notes(state);
					wl_surface_attach(state->wl_surface_popup, buffer_notes, 0, 0);
					wl_surface_damage_buffer(state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
					wl_surface_commit(state->wl_surface_popup);
				}
				else {
					/// get the current selected line
					currentSelectedLine = (state->y_motion / 33) + text_scroll;
					clipboardBuffer = returnlinefromfile(notesTXTPath, currentSelectedLine);
					///printf("Current Selected Line: %s\n", clipboardBuffer);
					/// trigger clipboard populate with data from file
					zwlr_data_control_device_v1_set_selection(state->zwlr_data_control_device_v1,
															state->zwlr_data_control_source_v1);
					state->zwlr_data_control_source_v1 = NULL;
					/// if remove line was clicked
					if (removeLineClicked) {
						/// remove selected line
						replacenthline(notesTXTPath, currentSelectedLine + 1, "");
					}
					struct wl_buffer *buffer_notes = draw_frame_notes(state);
					wl_surface_attach(state->wl_surface_popup, buffer_notes, 0, 0);
					wl_surface_damage_buffer(state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
					wl_surface_commit(state->wl_surface_popup);
				}
			}
			/// clicking on network icon
			if (strcmp(selectedWidget, "network") == 0) {
				///printf("Clicked inside network\n");
				if (tempTextBuff == NULL) {
					tempTextBuff = malloc(sizeof(char) * 2048);
					hiddenPassword = malloc(sizeof(char) * 2048);
					strcpy(tempTextBuff, "_");
					strcpy(hiddenPassword, "_");
				}
				int selectedNetwork = (state->y_motion / 70 + text_scroll_net);
				static char passSSID[300];
				/// clicking inside network dialog
				if (!securedNetwork) {
					///printf("Clicked inside network dialog\n");
					/// clicking on connect/disconnect button
					if (state->x_motion >= 450 && state->y_motion >= 10 && state->y_motion <=650) {
						///printf("selected network: %d\n", selectedNetwork);
						///printf("sel network name: %s\n", network_ptr->ssids[selectedNetwork]);
						///printf("sel network key: %s\n", network_ptr->keys[selectedNetwork]);
						netDialogPageDown = true;
						ssidsReady = true;
	
						/// check if selected network has been previously saved
						/// getting the command from config to check if saved
						char *checkIfSavedCMD = get_char_value_from_conf(config, "check_if_saved");
						char checkIfSavedCMDFull[strlen(checkIfSavedCMD) + \
												strlen(network_ptr->ssids[selectedNetwork]) + 100];
						snprintf(checkIfSavedCMDFull, sizeof(checkIfSavedCMDFull), \
																		"%s \"%s\" || echo ''", \
											checkIfSavedCMD, network_ptr->ssids[selectedNetwork]);
						///fprintf(stderr, "checkIfSavedCMDFull: %s\n", checkIfSavedCMDFull);
						const char *checkIfNetworkSaved = output_to_char(checkIfSavedCMDFull);
						static bool isNetworkSaved = false;
						/// check if the output of checking command returns selected network name
						///fprintf(stderr, "checkIfNetworkSaved: %s\n", checkIfNetworkSaved);
						///fprintf(stderr, "selectedNetwork: %s\n",
						///									network_ptr->ssids[selectedNetwork]);
						if (strcmp(checkIfNetworkSaved, network_ptr->ssids[selectedNetwork]) == 0) {
							///fprintf(stderr, "set %s as saved\n",
							///						network_ptr->ssids[selectedNetwork]);
							isNetworkSaved = true;
						}
						else {
							free((void *)checkIfNetworkSaved);
							checkIfNetworkSaved = NULL;
						}
						if (checkIfNetworkSaved != NULL) {
							free((void *)checkIfNetworkSaved);
							checkIfNetworkSaved = NULL;
						}
						if (checkIfSavedCMD != NULL) {
							free(checkIfSavedCMD);
							checkIfSavedCMD = NULL;
						}
						// If currently clicked network is not free
						if ((strcmp(network_ptr->keys[selectedNetwork], "free") != 0)) {
							///fprintf(stderr, "Secured network: %s\n",
							///								network_ptr->keys[selectedNetwork]);
							// We know this network is secured but before proceeding
							// we also need to find out if this network is currently active one
							// if clicked network if the currently active one
							// then we disconnect it.
							if (strcmp(network_ptr->ssids[selectedNetwork],
																	currActiveNetwork) == 0 && \
																	!isNetworkSaved) {
								/// getting disconnect command
								isNetworkSaved = false;
								char *disconnectCMD = get_char_value_from_conf(config,
																			"disconnect_cmd");
								char disconnectCMDFull[strlen(disconnectCMD) + \
												strlen(network_ptr->ssids[selectedNetwork]) + 10];
								snprintf(disconnectCMDFull, sizeof(disconnectCMDFull), "%s \"%s\"",
									disconnectCMD, network_ptr->ssids[selectedNetwork]);
								/// run disconnect command
								/// disconnect this network
								///fprintf(stderr, "disconnecting:%s\n",
								///							network_ptr->ssids[selectedNetwork]);
								///fprintf(stderr, "disconnect command: %s\n", disconnectCMDFull);
								run_cmd(disconnectCMDFull);
								if (disconnectCMD) {
									free(disconnectCMD);
									disconnectCMD = NULL;
								}
								///close_popup(state);
								wl_surface_attach(state_ptr->wl_surface_popup, NULL, 0, 0);
								wl_surface_commit(state_ptr->wl_surface_popup);
								ssidsTimer = false;
								popupEntered = false;
								///panelClicked = false;
								return;
								///hotspotEnabled = false;
							}
							
							// Now we need to find out if this network has been previously saved
							// if it has beeen previously saved then we don't need a password 
							// prompt when connecting.
							if (isNetworkSaved) {
								fprintf(stderr, "one\n");
								// Setting securedNetwork = false disables password dialog
								isNetworkSaved = false;
								securedNetwork = false;
								// Connect to this network without password prompt
								///printf("this network has been previously saved\n");
								///printf("connecting to: %s\n",
								///						network_ptr->ssids[selectedNetwork]);
								/// getting command to connect to selected network
								char *connectCMD = get_char_value_from_conf(config,
																			"cmd_connect");
								char connectCMDFull[strlen(connectCMD) + \
												strlen(network_ptr->ssids[selectedNetwork]) + 10];
								snprintf(connectCMDFull, sizeof(connectCMDFull), "%s \"%s\"",
									connectCMD, network_ptr->ssids[selectedNetwork]);
								/// run disconnect command
								run_cmd(connectCMDFull);
								if (connectCMD) {
									free(connectCMD);
									connectCMD = NULL;
								}
								///close_popup(state);
								wl_surface_attach(state_ptr->wl_surface_popup, NULL, 0, 0);
								wl_surface_commit(state_ptr->wl_surface_popup);
								ssidsTimer = false;
								popupEntered = false;
								return;
							}
							else {
								///printf("this network is secured, please enter the password\n");
								ssidsTimer = false;
								securedNetwork = true;
								snprintf(passSSID,
										sizeof(passSSID),
										"\"%s\"",
										network_ptr->ssids[selectedNetwork]);
								// Connect to this network with password prompt
								struct wl_buffer *buffer = draw_frame_password(state);
								wl_surface_attach(state->wl_surface_popup, buffer, 0, 0);
								wl_surface_damage_buffer(state->wl_surface_popup, 0, 0,
																	INT32_MAX, INT32_MAX);
								wl_surface_commit(state->wl_surface_popup);
								///close_popup(state);
								return;
							}
						}
						else { // If currently clicked network is free
							///fprintf(stderr, "Currently clicked SSID is a free network\n");
							// Connecti without password prompt
							securedNetwork = false;
							// Connect to this network without password prompt
							///printf("connecting: %s\n", network_ptr->ssids[selectedNetwork]);
							/// getting command to connect to selected network
							char *connectCMD = get_char_value_from_conf(config, "cmd_connect");
							char connectCMDFull[strlen(connectCMD) + \
												strlen(network_ptr->ssids[selectedNetwork]) + 10];
							snprintf(connectCMDFull, sizeof(connectCMDFull), "%s \"%s\"",
												connectCMD, network_ptr->ssids[selectedNetwork]);
							/// run disconnect command
							run_cmd(connectCMDFull);
							if (connectCMD != NULL) {
								free(connectCMD);
								connectCMD = NULL;
							}
							close_popup(state);
						}
					}
					if (state->y_motion >= 650 && state->x_motion <= 70) {
						/// toggle enable/disable
						/// getting wifi state enabled or disabled
						char *wifiStateCMD = get_char_value_from_conf(config, "wifi_state");
						const char *wifiState = output_to_char((const char *)wifiStateCMD);
						if (strcmp(wifiState, "enabled") != 0) {
							///printf("Wifi is Disabled\n");
							///printf("Enabling Wifi\n");
							wifiEnabled = true;
							/// getting enable wifi command from config
							char *wifiEnableCMD = get_char_value_from_conf(config, "enable_wifi");
							run_cmd(wifiEnableCMD);
							free(wifiEnableCMD);
							if (strcmp(wifiState, "enabled") != 0) {
								sleep(1);
							}
						}
						else {
							///printf("Wifi is Enabled\n");
							///printf("Disabling Wifi\n");
							wifiEnabled = false;
							/// getting disable wifi command from config
							char *wifiDisableCMD = get_char_value_from_conf(config,
																			"disable_wifi");
							run_cmd(wifiDisableCMD);
							free(wifiDisableCMD);
							if (strcmp(wifiState, "enabled") == 0) {
								sleep(1);
							}
						}
						free((void *)wifiState);
						free(wifiStateCMD);
						struct wl_buffer *buffer_network = draw_frame_network(state_ptr);
						wl_surface_attach(state_ptr->wl_surface_popup, buffer_network, 0, 0);
						wl_surface_damage_buffer(state_ptr->wl_surface_popup, 0, 0, INT32_MAX,
																					INT32_MAX);
						wl_surface_commit(state_ptr->wl_surface_popup);
					}
					if (state->y_motion >= 650 && \
							state->x_motion >= 90 && state->x_motion <= 290) {
						/// toggle enable/disable hotspot
						if (!hotspotEnabled) {
							///printf("HotSpot Enabled\n");
							hotspotEnabled = true;
							/// check if hotspot was previously created to skip creating new	
							char *checkIfSavedCMD = get_char_value_from_conf(config,
																				"check_if_saved");
							char checkIfSavedCMDFull[strlen(checkIfSavedCMD) + \
																strlen("quick-hotspot") + 10];
							snprintf(checkIfSavedCMDFull, sizeof(checkIfSavedCMDFull),
													"%s %s", checkIfSavedCMD, "quick-hotspot");
							const char *checkIfNetworkSaved = output_to_char(checkIfSavedCMDFull);
							static bool isNetworkSaved = false;
							/// check if output of checking command returns selected network name
							if (strcmp(checkIfNetworkSaved, "quick-hotspot") == 0) {
								isNetworkSaved = true;
							}
							else {
								free((void *)checkIfNetworkSaved);
								checkIfNetworkSaved = NULL;
							}
							if (!isNetworkSaved) {
								/// create hotspot
								/// get hotspot create command
								char *createHotspotCMD = get_char_value_from_conf(config,
																				"create_hotspot");
								run_cmd(createHotspotCMD);
								/// get activa hotspot command
								char *activateHotspotCMD = get_char_value_from_conf(config,
																			"activate_hotspot");
								/// connect to hotspot
								sleep(10); /// watiting until hotspot is created
								run_cmd(activateHotspotCMD);
								/// free memory
								if (createHotspotCMD != NULL) {
									free((void *)activateHotspotCMD);
									free((void *)createHotspotCMD);
									createHotspotCMD = NULL;
									activateHotspotCMD = NULL;
								}
							}
							else {
								/// connect to hotspot
								/// get activa hotspot command
								char *activateHotspotCMD = get_char_value_from_conf(config,
																			"activate_hotspot");
								/// connect to hotspot
								run_cmd(activateHotspotCMD);
								/// free memory
								if (activateHotspotCMD != NULL) {
									free((void *)activateHotspotCMD);
									activateHotspotCMD = NULL;
								}
							}
							if (checkIfNetworkSaved != NULL) {
								free((void *)checkIfNetworkSaved);
								checkIfNetworkSaved = NULL;
							}
							if (checkIfSavedCMD != NULL) {
								free(checkIfSavedCMD);
								checkIfSavedCMD = NULL;
							}
						}
						else {
							///printf("HotSpot Disabled\n");
							hotspotEnabled = false;
							/// disconnect hotspot
							/// getting command to disconnect hotspot
							char *disconnectCMD = get_char_value_from_conf(config,
																			"disconnect_cmd");
							char disconnectCMDFull[strlen(disconnectCMD) + \
																strlen("quick-hotspot") + 10];
							snprintf(disconnectCMDFull, sizeof(disconnectCMDFull), "%s %s",
																disconnectCMD, "quick-hotspot");
							/// run disconnect command
							run_cmd(disconnectCMDFull);
							free(disconnectCMD);
						}
						struct wl_buffer *buffer_network = draw_frame_network(state_ptr);
						wl_surface_attach(state_ptr->wl_surface_popup, buffer_network, 0, 0);
						wl_surface_damage_buffer(state_ptr->wl_surface_popup, 0, 0, INT32_MAX,
																					INT32_MAX);
						wl_surface_commit(state_ptr->wl_surface_popup);
					}
					if (state->y_motion >= 650 && state->x_motion >= 300 && \
																		state->x_motion <= 500){
						///printf("opening network manager\n");
						nmanagerOpen = true;
						char *networkManCMD = get_char_value_from_conf(config, "nm_manager");
						if (networkManCMD != NULL) {
							run_cmd(networkManCMD);
							free(networkManCMD);
							networkManCMD = NULL;
								ssidsTimer = false;
								popupEntered = false;
								nmanagerOpen = false;
						}
					}
					if (state->y_motion >= 670 && state->x_motion >= 570) {
						///printf("Network page down\n");
						netItemsCounter = netItemsCounter + 9;
						text_scroll_net = text_scroll_net + 9;
						netDialogPageDown = true;
						ssidsReady = true;
					}
					if (state->y_motion >= 640 && \
							state->y_motion < 670 && state->x_motion >= 570) {
						///printf("Network page up\n");
						netItemsCounter = netItemsCounter - 9;
						text_scroll_net = text_scroll_net - 9;
						netDialogPageDown = true;
						ssidsReady = true;
					}
					struct wl_buffer *buffer_network = draw_frame_network(state_ptr);
					wl_surface_attach(state_ptr->wl_surface_popup, buffer_network, 0, 0);
					wl_surface_damage_buffer(state_ptr->wl_surface_popup, 0, 0, INT32_MAX,
																				INT32_MAX);
					wl_surface_commit(state_ptr->wl_surface_popup);
				}
				else {
					/// inside password dialog clicking
					///printf("clicked inside password window\n");
					///printf("x %d y %d\n", state->x_motion, state->y_motion);
					if (state->x_motion >= 155 && state->x_motion < 330 && \
											state->y_motion >= 185 && state->y_motion < 220) {
						///printf("cancel password clicked\n");
						close_popup(state);
					}
					else if (state->x_motion > 355 && state->x_motion < 545 && \
											state->y_motion >= 185 && state->y_motion < 220) {
						///printf("connect password clicked\n");
						///printf("connectng to: %s\n", passSSID);
						///printf("with password: %s\n", tempTextBuff);
						/// connecting to secure network command
						/// getting command to connect to the selected secure network
						char *connectSecureCMD = get_char_value_from_conf(config, "cmd_connect");
						char connectSecureCMDFull[strlen(connectSecureCMD) + \
																		strlen(passSSID) + 10];
						snprintf(connectSecureCMDFull, 2048, "%s %s password \"%s\"",
													connectSecureCMD, passSSID, tempTextBuff);
						///printf("secured conn: %s\n", connectSecureCMDFull);
						/// run disconnect command
						run_cmd(connectSecureCMDFull);
						free(connectSecureCMD);
						connectSecureCMD = NULL;
						free(tempTextBuff);
						tempTextBuff = NULL;
						/// Fill str with zeroes
						memset(connectSecureCMDFull, 0, sizeof(connectSecureCMDFull));
						close_popup(state);
					}
					else if (state->x_motion >= 630 && state->x_motion < 670 && \
											state->y_motion >= 100 && state->y_motion < 130) {
						if (!showPassword) {
							///printf("show password on\n");
							showPassword = true;
						}
						else {
							///printf("show password off\n");
							showPassword = false;
						}
					}
				}
			}
		}
	}

	if (event->event_mask & POINTER_EVENT_MOTION) {
		//fprintf(stderr, "x=%d\n", state->x_motion);
		state->x_motion = wl_fixed_to_int(event->surface_x);
		/// FIX mouse move on panel altering popup, parse y motion only when popup entered
		if (popupEntered) {
			state->y_motion = wl_fixed_to_int(event->surface_y);
		}
	}

	///fprintf(stderr, "y\n");
	memset(event, 0, sizeof(*event));
}

static const struct wl_pointer_listener wl_pointer_listener = {
	.enter = wl_pointer_enter,
	.leave = wl_pointer_leave,
	.motion = wl_pointer_motion,
	.button = wl_pointer_button,
	.axis = wl_pointer_axis,
	.frame = wl_pointer_frame,
	.axis_source = noop,
	.axis_stop = wl_pointer_axis_stop,
	.axis_discrete = wl_pointer_axis_discrete,
};

/* Keyboard Managing */
// This block sets the initial keymap
static void wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard, uint32_t format,
																	int32_t fd, uint32_t size) {
	///printf("group: %d\n", a);
	(void)wl_keyboard;
	struct client_state *state = data;
	assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);
	char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	assert(map_shm != MAP_FAILED);
	struct xkb_keymap *xkb_keymap;
	xkb_keymap = xkb_keymap_new_from_string(state->xkb_context, map_shm,
										XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap(map_shm, size);
	close(fd);

	struct xkb_state *xkb_state = xkb_state_new(xkb_keymap);
	xkb_keymap_unref(state->xkb_keymap);
	xkb_state_unref(state->xkb_state);
	state->xkb_keymap = xkb_keymap;
	state->xkb_state = xkb_state;
}

/// This block is executed when you open notes window and pressing a key on keyboard
static void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial,
													uint32_t time, uint32_t key, uint32_t state) {
	(void)wl_keyboard;
	(void)serial;
	(void)time;
	/// in none of the input text dialogs opened then exit
	if (!addNewNote && !securedNetwork) {
		return;
	}
	struct client_state *client_state = data;
	char buf[128];
	struct wl_buffer *buffer_n;
	uint32_t keycode = key + 8;
	const char *action = state == WL_KEYBOARD_KEY_STATE_PRESSED ? "press" : "release";
	xkb_state_key_get_utf8(client_state->xkb_state, keycode, buf, sizeof(buf));
	///fprintf(stderr, "utf8: '%s'\n", buf);
	xkb_keysym_t sym = xkb_state_key_get_one_sym(client_state->xkb_state, keycode);
	if (sym == XKB_KEY_Shift_L || sym == XKB_KEY_Shift_R || sym == XKB_KEY_Control_L || \
		sym == XKB_KEY_Control_R || sym == XKB_KEY_Alt_L || sym == XKB_KEY_Alt_R || \
		sym == XKB_KEY_Super_L || sym == XKB_KEY_Super_R || sym == XKB_KEY_Tab || \
		sym == XKB_KEY_Caps_Lock) {
		///printf("Modifiers pressed, do nothing\n");
		return;
	}
	
	FILE *file;
	/// if add new note was clicked
	if (addNewNote) {
		file = fopen(notesTXTPath, "a"); // Open file in append mode
	}
	///int length = xkb_keysym_to_utf8(sym, buf, sizeof(buf));
	/// action on a key pressed on the keyboard
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED && strcmp(action, "press") == 0) {
		if (sym == XKB_KEY_BackSpace) {
			///printf("BackSpace pressed, removing last char\n");
			if (strlen(tempTextBuff) > 0) {
				if (tempTextBuff != NULL) {
					tempTextBuff[strlen(tempTextBuff) - 1] = '\0';
				}
				if (hiddenPassword != NULL) {
					hiddenPassword[strlen(hiddenPassword) - 1] = '\0';
				}
			}
			/// showing new notes text as you typw
			if (!securedNetwork) {
				buffer_n = draw_frame_notes(client_state);
			}
			else {
				buffer_n = draw_frame_password(client_state);
			}
			wl_surface_attach(client_state->wl_surface_popup, buffer_n, 0, 0);
			wl_surface_damage_buffer(client_state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
			wl_surface_commit(client_state->wl_surface_popup);

			buf[0] = '\0';

			if (!securedNetwork) {
				/// removing last char from the temporary text buffer
				fseek(file, -1, SEEK_END);
				/// Truncate the file at the current position
				int result = ftruncate(fileno(file), ftell(file));
				if (result != 0) {
					perror("Error truncating file");
				}
				fclose(file); // Close the file after writing
			}
			return;
		}
		else {
			/// showing temproraty buffer white typing the new note
			if (!securedNetwork && strcmp(tempTextBuff, "|type the new note here|") == 0) {
				strncpy(tempTextBuff, buf, 1);
				tempTextBuff[1] = '\0';
				fprintf(file, "%s", buf); // Append the typed letter to the file
				fclose(file); // Close the file after writing
			}
			else if (!securedNetwork && strcmp(tempTextBuff, "|type the new note here|") != 0) {
				strcat(tempTextBuff, buf);
				fprintf(file, "%s", buf); // Append the typed letter to the file
				fclose(file); // Close the file after writing
			}
			/// showing new notes text as you typw
			if (!securedNetwork) {
				buffer_n = draw_frame_notes(client_state);
			}
			else {
				/// this clock is executed when password prompt is opened
				if (strcmp(tempTextBuff, "_") == 0) {
					tempTextBuff[0] = '\0';
					hiddenPassword[0] = '\0';
					strcat(tempTextBuff, buf);
					strcat(hiddenPassword, "*");
				}
				else {
					strcat(tempTextBuff, buf);
					strcat(hiddenPassword, "*");
				}
				buffer_n = draw_frame_password(client_state);
			}
			wl_surface_attach(client_state->wl_surface_popup, buffer_n, 0, 0);
			wl_surface_damage_buffer(client_state->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
			wl_surface_commit(client_state->wl_surface_popup);
			return;
		}
	}
}


// This block is used to indicate the currently used keyboard layout (aka group)
static void wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard, 
			uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
																				uint32_t group) {
	(void)wl_keyboard;
	(void)serial;
	/// getting layout
	struct client_state *client_state = data;
	client_state->xkb_group = group;
	xkb_state_update_mask(client_state->xkb_state, mods_depressed, mods_latched,
																	mods_locked, 0, 0, group);
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
	.keymap = wl_keyboard_keymap,
	.enter = noop,
	.leave = noop,
	.key = wl_keyboard_key,
	.modifiers = wl_keyboard_modifiers,
	.repeat_info = noop,
};

/* Clipboard Data Control Device */
static void zwlr_data_control_device_v1_selection(void *data,
								struct zwlr_data_control_device_v1 *zwlr_data_control_device_v1,
														struct zwlr_data_control_offer_v1 *id) {
	(void)data;
	(void)zwlr_data_control_device_v1;
	(void)id;
	///printf("Selection Request Sent\n");
	struct client_state *state = data;
	state->zwlr_data_control_device_v1 = zwlr_data_control_device_v1;
	state->zwlr_data_control_offer_v1 = id;

	/// creating source object
	state->zwlr_data_control_source_v1 = zwlr_data_control_manager_v1_create_data_source(
															state->zwlr_data_control_manager_v1);
	zwlr_data_control_source_v1_add_listener(state->zwlr_data_control_source_v1,
					 								&zwlr_data_control_source_v1_listener, state);
	zwlr_data_control_source_v1_offer(state->zwlr_data_control_source_v1,
																	"text/plain;charset=utf-8");
}

struct zwlr_data_control_device_v1_listener zwlr_data_control_device_v1_listener = {
	.data_offer = noop,
	.selection = zwlr_data_control_device_v1_selection,
	.finished = noop,
	.primary_selection = noop,
};

// Sending the data from temporary file to clipboard
static void zwlr_data_control_source_v1_send(void *data,
		struct zwlr_data_control_source_v1 *zwlr_data_control_source_v1, const char *mime_type,
																					int32_t fd) {
	(void)data;
	(void)mime_type;
	(void)zwlr_data_control_source_v1;
	///printf("Data Sent to Clipboard\n");
	///printf("clipboardBuffer: %s\n", clipboardBuffer);
	/// write data to clipboard
	/// Check if fd is valid before writing
	if (fd >= 0 && clipboardBuffer != NULL) {
		write(fd, clipboardBuffer, strlen(clipboardBuffer));
	}
	else {
		perror("Write error");
	}
	close(fd);
}

static void zwlr_data_control_source_v1_send_cancelled(void *data,
							struct zwlr_data_control_source_v1 *zwlr_data_control_source_v1) {
	(void)data;
	(void)zwlr_data_control_source_v1;
	///printf("Control Source Cancelled\n");
	zwlr_data_control_source_v1_destroy(zwlr_data_control_source_v1);
	zwlr_data_control_source_v1 = NULL;
}

struct zwlr_data_control_source_v1_listener zwlr_data_control_source_v1_listener = {
	.send = zwlr_data_control_source_v1_send,
	.cancelled = zwlr_data_control_source_v1_send_cancelled,
};

/* Getting current time function */
static char *getcurrenttime() {
	/// returns microseconds
	gint64 realtime = g_get_real_time();
	/// Convert microseconds to seconds
	gint64 seconds = realtime / G_USEC_PER_SEC;
	GDateTime *datetime;
	datetime = g_date_time_new_from_unix_local(seconds);
	gchar *currenttime = g_date_time_format(datetime, "%R");
	g_date_time_unref(datetime);
	return currenttime;
}

/************************************************************************************************/
/*************************** DRAWING VOLUME BRUGHTNESS POPUP ************************************/
/************************************************************************************************/
static void wl_buffer_release_popup(void *data, struct wl_buffer *wl_buffer) {
	/* Sent by the compositor when it's no longer using this buffer */
	(void)data;
	wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener_popup = {
	.release = wl_buffer_release_popup,
};

static struct wl_buffer *draw_frame_volume_bright_popup(struct client_state *state) {
	int width = state->width_popup;
	int height = state->height_popup;
	int stride = width * 4;
	int size = stride * height;
	int fd = allocate_shm_file(size);

	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	cairo_surface_t *surface = cairo_image_surface_create_for_data((unsigned char *)data,
																		CAIRO_FORMAT_RGB24,
																		width,
																		height,
																		stride);
	cairo_t *cr = cairo_create(surface);
	cairo_paint(cr);

	/// grey color
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);

	/// white grey color
	cairo_set_source_rgba(cr, 0.4, 0.4, 0.5, 1.0);
	cairo_rectangle(cr, 11, 42, 7, height);
	cairo_fill(cr);

	/// controlling the blue bar
	/// volume blue bar
	if (strcmp(selectedWidget, "volume") == 0) {
		cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 1.0);
		cairo_rectangle(cr, 11, (130 - get_current_volume_level()), 7, height);
		cairo_fill(cr);
	}
	/// brightness blue bar
	if (strcmp(selectedWidget, "brightness") == 0) {
		const char *path = get_char_value_from_conf(config, "brightness_file_path");
		cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 1.0);
		cairo_rectangle(cr, 11, (130 - get_current_brightness(path)), 7, height);
		cairo_fill(cr);
		free((void *)path);
	}

	/// grey color
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	cairo_rectangle(cr, 0, 130, width, height);
	cairo_fill(cr);

	/// draw frame around
	cairo_set_source_rgba(cr, 0.0, 0.7, 1.0, 1.0);
	cairo_set_line_width(cr, 0.5);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_stroke(cr);

	/// drawing lower minus
	cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 1.0);
	cairo_set_line_width(cr, 3);
	cairo_move_to(cr, 7, 150);
	cairo_line_to(cr, 23, 150);
	cairo_stroke(cr);

	/// drawing upper plus
	cairo_set_source_rgba(cr, 0.0, 0.0, 1.0, 1.0);
	cairo_set_line_width(cr, 3);
	cairo_move_to(cr, 7, 21);
	cairo_line_to(cr, 23, 21);
	cairo_stroke(cr);

	cairo_move_to(cr, 15, 12);
	cairo_line_to(cr, 15, 29.9);
	cairo_stroke(cr);

	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
																	WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);

	munmap(data, size);
	wl_buffer_add_listener(buffer, &wl_buffer_listener_popup, NULL);
	return buffer;
}

/************************************************************************************************/
/************************************** DRAWING NETWORK *****************************************/
/************************************************************************************************/
/* Timer to populate and display the available networks SSIDs and security status */
static void show_networks() {
	// If network manager is already open, close it and return.
	if (nmanagerOpen) {
		nmanagerOpen = false;
		return;
	}

	// Ensure network_ptr is not NULL to avoid dereferencing a NULL pointer.
	if (network_ptr == NULL) {
		fprintf(stderr, "Error: network_ptr is NULL.\n");
		return;
	}

	// Initialize SSIDs array to NULL to avoid undefined behavior.
	network_ptr->ssids[0] = NULL;

	// Get network scan commands.
	char *networkScanCMD = get_char_value_from_conf(config, "network_scan");
	char *networkScanKeyCMD = get_char_value_from_conf(config, "network_key_scan");

	// Check if the commands were retrieved successfully.
	if (networkScanCMD == NULL || networkScanKeyCMD == NULL) {
		fprintf(stderr, "Error: Failed to retrieve network scan commands.\n");
		free(networkScanCMD);
		free(networkScanKeyCMD);
		return;
	}

	// Populate the SSIDs and keys arrays.
	output_to_char_array(network_ptr->ssids, networkScanCMD);
	output_to_char_array(network_ptr->keys, networkScanKeyCMD);

	// Retry scanning if no SSIDs were found, panel was clicked, and it's not a secured network.
	if (network_ptr->ssids[0] == NULL && panelClicked && !securedNetwork) {
		output_to_char_array(network_ptr->ssids, networkScanCMD);
		output_to_char_array(network_ptr->keys, networkScanKeyCMD);
	}

	// Refresh the frame buffer if SSIDs are ready, panel was clicked, and it's not a secured
	// network.
	if (network_ptr->ssids[0] != NULL && panelClicked && !securedNetwork) {
		ssidsReady = true;

		// Ensure state_ptr is not NULL.
		if (state_ptr == NULL) {
			fprintf(stderr, "Error: state_ptr is NULL.\n");
			free(networkScanCMD);
			free(networkScanKeyCMD);
			return;
		}

		struct wl_buffer *buffer_network = draw_frame_network(state_ptr);
		if (buffer_network == NULL) {
			fprintf(stderr, "Error: Failed to draw network frame.\n");
			free(networkScanCMD);
			free(networkScanKeyCMD);
			return;
		}

		wl_surface_attach(state_ptr->wl_surface_popup, buffer_network, 0, 0);
		wl_surface_damage_buffer(state_ptr->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
		wl_surface_commit(state_ptr->wl_surface_popup);
	}

	// Free allocated memory for the scan commands.
	free(networkScanCMD);
	free(networkScanKeyCMD);
}


static struct wl_buffer *draw_frame_password(struct client_state *state) {
	int width = 700;
	int height = 270;
	int stride = width * 4;
	int size = stride * height;
	int fd = allocate_shm_file(size);

	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
																	WL_SHM_FORMAT_ARGB8888);
	
	cairo_surface_t *surface = cairo_image_surface_create_for_data((unsigned char *)data,
																		CAIRO_FORMAT_RGB24,
																		width,
																		height,
																		stride);
	cairo_t *cr = cairo_create(surface);
	cairo_paint(cr);

	/// grey color
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);

	/// draw frame around
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 3);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_stroke(cr);

	/// draw enter password text
	cairo_set_font_size(cr, 25.0);
	cairo_move_to(cr, 190, 60);
	cairo_show_text(cr, "Please enter the password");

	/// draw password prompt
	cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, 20, 100, width - 40, 50);
	cairo_fill(cr);

	/// draw password prompt frame
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, 20, 100, width - 40, 50);
	cairo_stroke(cr);

	/// draw show password button
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	cairo_rectangle(cr, (width - 60), 110, 30, 30);
	cairo_fill(cr);

	/// draw show password frame
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, (width - 60), 110, 30, 30);
	cairo_stroke(cr);

	/// draw letter w for show password
	cairo_set_font_size(cr, 20.0);
	cairo_move_to(cr, (width - 52), 130);
	cairo_show_text(cr, "w");

	/// draw cancel button frame
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, 145, (height - 90), 200, 50);
	cairo_stroke(cr);
	cairo_set_font_size(cr, 25.0);
	cairo_move_to(cr, 210, (height - 57));
	cairo_show_text(cr, "сancel");

	/// draw connect button frame
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, 355, (height - 90), 200, 50);
	cairo_stroke(cr);
	cairo_move_to(cr, 405, (height - 57));
	cairo_show_text(cr, "сonnect");

	/// typing the password
	cairo_set_font_size(cr, 23);
	cairo_move_to(cr, 30, 135);
	if (showPassword) {
		cairo_show_text(cr, tempTextBuff);
	}
	else {
		cairo_show_text(cr, hiddenPassword);
	}

	/// clear resources
	cairo_surface_flush(surface); // Flush changes to the surface
	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	wl_shm_pool_destroy(pool);
	close(fd);

	munmap(data, size);
	wl_buffer_add_listener(buffer, &wl_buffer_listener_popup, NULL);
	return buffer;
}

static struct wl_buffer *draw_frame_network(struct client_state *state) {
	int width = 600;
	int height = 700;
	int stride = width * 4;
	int size = stride * height;
	int fd = allocate_shm_file(size);

	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
																	WL_SHM_FORMAT_ARGB8888);
	
	cairo_surface_t *surface = cairo_image_surface_create_for_data((unsigned char *)data,
																		CAIRO_FORMAT_RGB24,
																		width,
																		height,
																		stride);
	cairo_t *cr = cairo_create(surface);
	cairo_paint(cr);

	/// grey color
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);

	/// draw frame around
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 3);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_stroke(cr);

	/// draw smaller frame
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, 10, 10, width - 20, height - 75);
	cairo_stroke(cr);

	/// drawing lower cover
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	cairo_rectangle(cr, 3, (height - 63), (width - 5), 62);
	cairo_fill(cr);

	/// if enabled/disabled switch
	if (wifiEnabled && !securedNetwork) {
		/// draw enabled switch
		cairo_set_source_rgba(cr, 0.0, 0.8, 1.0, 1.0); /// blue
		cairo_rectangle(cr, 10, (height - 41), 60, 17);
		cairo_fill(cr);
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0); /// white
		cairo_set_line_width(cr, 1);
		cairo_rectangle(cr, 10, (height - 41), 60, 17);
		cairo_stroke(cr);
		cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 1.0); /// dark
		cairo_arc(cr, 60, (height - 33), 15, 0, 2 * M_PI); /// circle
		cairo_fill(cr);
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0); /// white
		cairo_arc(cr, 60, (height - 33), 15, 0, 2 * M_PI); /// circle
		cairo_stroke(cr);
		/// show networks only when the SSIDs are populated and ready
		if (ssidsTimer && !netDialogPageDown) {
			///printf("timer started\n");
			async_timer(show_networks, 3);
			ssidsTimer = false;
		}
		if (ssidsReady) {
			///printf("ssids ready started\n");
			int text_pos_y = 0;
			int frame_pos_y = 20;
			for (int i = netItemsCounter; i < (text_scroll_net + 9); i++) {
				if (network_ptr->ssids[i] == NULL) {
					break;
				}

				/// drawing frame around SSID text
				if (frame_pos_y < 605) { /// limit the number of squares to 8
					cairo_set_line_width(cr, 1);
					cairo_rectangle(cr, 20, frame_pos_y, (width - 42), 55);
					cairo_stroke(cr);
					/// drawing connect/disconnect buttons
					cairo_rectangle(cr, (width - 150), (frame_pos_y + 12), 120, 30);
					cairo_stroke(cr);
					
					/// getting the currently active network position
					if (strcmp(network_ptr->ssids[i], currActiveNetwork) == 0) {
						cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 1.0); /// green
						/// drawing SSID
						text_pos_y = text_pos_y + 47;
						/// drawing text
						cairo_set_font_size(cr, 20.0);
						cairo_move_to(cr, 30, text_pos_y);
						cairo_show_text(cr, network_ptr->ssids[i]);
						/// drawing security status (free or private)
						text_pos_y = text_pos_y + 20;
						cairo_set_font_size(cr, 15.0);
						cairo_move_to(cr, 30, text_pos_y);
						cairo_show_text(cr, network_ptr->keys[i]);
						/// drawing disconnect text
						cairo_set_font_size(cr, 21.0);
						cairo_move_to(cr, (width - 144), (frame_pos_y + 32));
						cairo_show_text(cr, "disconnect");
						currAciveNetworkPos = i;
						cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0); /// white
					}
					else {
						cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0); /// white
						/// drawing SSID
						text_pos_y = text_pos_y + 47;
						/// drawing text
						cairo_set_font_size(cr, 20.0);
						cairo_move_to(cr, 30, text_pos_y);
						cairo_show_text(cr, network_ptr->ssids[i]);
						/// drawing security status (free or private)
						text_pos_y = text_pos_y + 20;
						cairo_set_font_size(cr, 15.0);
						cairo_move_to(cr, 30, text_pos_y);
						cairo_show_text(cr, network_ptr->keys[i]);
						/// drawing connect text
						cairo_set_font_size(cr, 20.0);
						cairo_move_to(cr, (width - 130), (frame_pos_y + 32));
						cairo_show_text(cr, "connect");
					}

					frame_pos_y = frame_pos_y + 67;
				}
			}

			ssidsReady = false;
		}
		else {
			/// scanning for available networks text
			cairo_move_to(cr, 40, (height / 2));
			cairo_set_font_size(cr, 30.0);
			cairo_show_text(cr, "Scanning for networks please wait...");
		}
	}
	else if (!wifiEnabled && !securedNetwork) {
		/// draw disabled switch
		cairo_set_source_rgba(cr, 0.4, 0.4, 0.5, 1.0); /// white gray
		cairo_rectangle(cr, 10, (height - 41), 60, 17);
		cairo_fill(cr);
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0); /// white
		cairo_set_line_width(cr, 1);
		cairo_rectangle(cr, 10, (height - 41), 60, 17);
		cairo_stroke(cr);
		cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 1.0); /// dark
		cairo_arc(cr, 22, (height - 32), 15, 0, 2 * M_PI); /// circle
		cairo_fill(cr);
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0); /// white
		cairo_arc(cr, 22, (height - 32), 15, 0, 2 * M_PI); /// circle
		cairo_stroke(cr);
		/// scanning for available networks text
		cairo_move_to(cr, 190, (height / 2));
		cairo_set_font_size(cr, 30.0);
		cairo_show_text(cr, "WIFI is disabled!");
	}

	/// drawing hotspot button
	if (hotspotEnabled) {
		cairo_set_source_rgba(cr, 0.0, 0.8, 1.0, 1.0); /// blue
		cairo_set_line_width(cr, 1);
		cairo_rectangle(cr, 90, (height - 47), 200, 30);
		cairo_stroke(cr);
		cairo_move_to(cr, 117, (height - 27));
		cairo_set_font_size(cr, 20.0);
		cairo_show_text(cr, "Disable HotSpot");
	}
	else {
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0); /// white
		cairo_set_line_width(cr, 1);
		cairo_rectangle(cr, 90, (height - 47), 200, 30);
		cairo_stroke(cr);
		cairo_move_to(cr, 117, (height - 27));
		cairo_set_font_size(cr, 20.0);
		cairo_show_text(cr, "Enable HotSpot");
	}

	/// drawing network manager button
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0); /// white
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, 300, (height - 47), 200, 30);
	cairo_stroke(cr);
	cairo_move_to(cr, 315, (height - 27));
	cairo_set_font_size(cr, 20.0);
	cairo_show_text(cr, "Network Manager");

	/// draw arrow up
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, (width - 25), (height - 50), width, height);
	cairo_set_line_width(cr, 3);
	/// right to left line
	cairo_move_to(cr, (width - 13), (height - 43));
	cairo_line_to(cr, (width - 21), (height - 33));
	/// left to right line
	cairo_move_to(cr, (width - 13), (height - 43));
	cairo_line_to(cr, (width - 5), (height - 33));
	cairo_stroke(cr);

	/// draw arrow down
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, (width - 25), (height - 25) , width, height);
	cairo_set_line_width(cr, 3);
	/// right to left line
	cairo_move_to(cr, (width - 21), (height - 18));
	cairo_line_to(cr, (width - 13), (height - 8));
	/// left to right line
	cairo_move_to(cr, (width - 5), (height - 18));
	cairo_line_to(cr, (width - 13), (height - 8));
	cairo_stroke(cr);

	/// clean resources
	cairo_surface_flush(surface); // Flush changes to the surface
	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	wl_shm_pool_destroy(pool);
	close(fd);

	munmap(data, size);
	wl_buffer_add_listener(buffer, &wl_buffer_listener_popup, NULL);
	return buffer;
}

/* This empty buffer is a hack and is used to open for a brief second a popup on mouse hover
 * this popup is used to retrieve the necessary keyboard info (e.g. group aka layout)
 */
/************************************************************************************************/
/******************************** DRAWING AN EMPTY BUFFER ***************************************/
/************************************************************************************************/
static struct wl_buffer *draw_frame_empty(struct client_state *state) {
	int width = 1;
	int height = 1;
	int stride = width * 4;
	int size = stride * height;
	int fd = allocate_shm_file(size);

	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
																	WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);

	munmap(data, size);
	wl_buffer_add_listener(buffer, &wl_buffer_listener_popup, NULL);
	return buffer;
}

/************************************************************************************************/
/************************************** DRAWING CALENDAR ****************************************/
/************************************************************************************************/
static struct wl_buffer *draw_frame_datetime_popup(struct client_state *state) {
	int width = 330;
	int height = 330;
	int stride = width * 4;
	int size = stride * height;
	int fd = allocate_shm_file(size);

	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	cairo_surface_t *surface = cairo_image_surface_create_for_data((unsigned char *)data,
																	CAIRO_FORMAT_RGB24,
																	width,
																	height,
																	stride);
	cairo_t *cr = cairo_create(surface);
	cairo_paint(cr);

	/// grey color
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);

    /// Get current time
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	///int current_month = timeinfo->tm_mon + 1; // Months are 0-based in tm struct
	///int current_year = timeinfo->tm_year + 1900; // Years are years since 1900
	int current_day = timeinfo->tm_mday; // Day of the month

	// Set font size and color for text
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 15.0);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White text color
	cairo_set_line_width(cr, 1.0); // Set a default line width for the grid

	// Calculate cell width and height for the calendar grid
	int cell_width = width / 7; // Assuming a week starts on Sunday
	int cell_height = height / 7;

	/// Calculate the day of the week for the first day of the month
	struct tm first_day_tm = *timeinfo;
	first_day_tm.tm_mday = 1;
	mktime(&first_day_tm);

	int first_day_of_week = first_day_tm.tm_wday; // 0 = Sunday, 1 = Monday, ...
	if (first_day_of_week == 0) { // Adjust for Monday being the first day
		first_day_of_week = 6; // Sunday becomes 6
	}
	else {
		first_day_of_week--; // Shift other days back by one
	}

	/// Draw calendar grid
	for (int row = 0; row < 6; row++) {
		for (int col = 0; col < 7; col++) {
			int day = (row * 7) + col - first_day_of_week + 1; // Day of the month
			if (day <= 31 && day >= 1) {
				int x = col * cell_width;
				int y = row * cell_height;
				cairo_rectangle(cr, x, y, cell_width, cell_height);
				cairo_stroke(cr); // Stroke the rectangle

				/// Highlight current day with stroke
				if (day == current_day) {
					cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); // Red color for stroke
					cairo_set_line_width(cr, 5.0); // Adjust the line width as needed
					/// Adjust the rectangle dimensions to fit within the stroke
					cairo_rectangle(cr, x + 1, y + 1, cell_width - 2, cell_height - 2);
					cairo_stroke(cr);
				}
				cairo_set_line_width(cr, 1.0); /// thickness of squares frame
				cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White text color
				cairo_move_to(cr, x + 5, y + 15); // Adjust text position within cell
				char day_str[4]; // Buffer to hold the day name (3 letters + null terminator)
				strftime(day_str, sizeof(day_str), "%a", &first_day_tm); // Format day name
				cairo_show_text(cr, day_str); // Display day name

                /// Display date number
                char date_str[3]; // Buffer to hold the date number (2 digits + null terminator)
                snprintf(date_str, sizeof(date_str), "%d", day); // Convert day to string
                cairo_move_to(cr, x + 5, y + 35); // Adjust text position for date number
				cairo_set_font_size(cr, 17.0); /// numbers size
                cairo_show_text(cr, date_str); // Display date number

				first_day_tm.tm_mday++; // Move to the next day for the next iteration
				mktime(&first_day_tm); // Update the time structure
			}
		}
	}

	char dateBuffer[128];
	strftime(dateBuffer, sizeof(dateBuffer), "%A, %d %B %Y", timeinfo);

	// Get the width of the widget and font size
	int widget_width = 330;
	double font_size = 20.0;

	// Set font size and calculate text extents
	cairo_set_font_size(cr, font_size);
	cairo_text_extents_t extents;
	cairo_text_extents(cr, dateBuffer, &extents);

	// Calculate x position to center the text in the widget
	double text_width = extents.width;
	double x_position = (widget_width - text_width) / 2.0;

	// Move to the calculated x position and show the text
	cairo_move_to(cr, x_position, 310);  // y-coordinate remains unchanged
	cairo_show_text(cr, dateBuffer);     // Display centered text

	/// draw frame around
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 3);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_stroke(cr);

	cairo_surface_flush(surface); // Flush changes to the surface
	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
																		WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);

	munmap(data, size);
	wl_buffer_add_listener(buffer, &wl_buffer_listener_popup, NULL);
	return buffer;
}

/************************************************************************************************/
/************************************ DRAWING NOTES *********************************************/
/************************************************************************************************/
static struct wl_buffer *draw_frame_notes(struct client_state *state) {
	int width = 600;
	int height = 530;
	int stride = width * 4;
	int size = stride * height;
	int fd = allocate_shm_file(size);

	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
																	WL_SHM_FORMAT_ARGB8888);

	cairo_surface_t *surface = cairo_image_surface_create_for_data((unsigned char *)data,
																		CAIRO_FORMAT_RGB24,
																		width,
																		height,
																		stride);
	cairo_t *cr = cairo_create(surface);
	cairo_paint(cr);

	/// grey color
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_fill(cr);

	/// draw frame around
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 3);
	cairo_rectangle(cr, 0, 0, width, height);
	cairo_stroke(cr);

	/// draw arrow up
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, (width - 25), (height - 50) , width, height);
	cairo_set_line_width(cr, 3);
	/// right to left line
	cairo_move_to(cr, (width - 13), (height - 43));
	cairo_line_to(cr, (width - 21), (height - 33));
	/// left to right line
	cairo_move_to(cr, (width - 13), (height - 43));
	cairo_line_to(cr, (width - 5), (height - 33));
	cairo_stroke(cr);

	/// clicking on add new note creates this buffer
	if (addNewNote) {
		cairo_set_font_size(cr, 25);
		cairo_move_to(cr, 5, 30);
		cairo_show_text(cr, tempTextBuff);
		text_scroll = 0; /// this is needed in order to show the new note as position zero
	}
	else {
		/// Read text from notes.txt
		FILE *file = fopen(notesTXTPath, "r");
		if (file == NULL) {
			printf("Error opening notes.txt\n");
		}

		if (file != NULL) {
			char line[4096]; // Assuming each line in the file is at most 255 characters long
			int y = 10; // Starting y position for text
			/// start reading the file after given nth line
			int lineCount = 0; // Counter for lines read
			while (fgets(line, sizeof(line), file) != NULL) {
				if (lineCount >= text_scroll) { // Start rendering from the 15th line
					/// Remove newline character if present
					size_t len = strlen(line);
					if (len > 0 && line[len - 1] == '\n') {
						line[len - 1] = '\0';
					}
					cairo_move_to(cr, 10, (y + 15));
					if (lineCount == currentSelectedLine) {
						cairo_set_source_rgb(cr, 0.0, 1.0, 1.0); // highlighted line
					}
					else {
						cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // White text color
					}
					cairo_set_font_size(cr, 25);
					cairo_show_text(cr, line);
					y = y + 33; // Increase y position for the next line
				}
				lineCount = lineCount + 1; // Increment line counter
			}
			fclose(file);
		}
	}

	/// drawing lower cover
	cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
	cairo_rectangle(cr, 3, (height - 30), (width - 5), 27);
	cairo_fill(cr);

	/// draw arrow down
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, (width - 25), (height - 25) , width, height);
	cairo_set_line_width(cr, 3);
	/// right to left line
	cairo_move_to(cr, (width - 21), (height - 18));
	cairo_line_to(cr, (width - 13), (height - 8));
	/// left to right line
	cairo_move_to(cr, (width - 5), (height - 18));
	cairo_line_to(cr, (width - 13), (height - 8));
	cairo_stroke(cr);

	/// drawing add new note button
	cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, 5, (height - 33) , 200, 30);
	cairo_stroke(cr);
	/// add nore text
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 20);
	cairo_move_to(cr, 37, (height - 12));
	cairo_show_text(cr, "Add New Note");

	/// drawing paste button
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, 220, (height - 33) , 200, 30);
	cairo_stroke(cr);
	/// paste here text
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_move_to(cr, 270, (height - 12));
	cairo_show_text(cr, "Paste Here");

	/// drawing remove button, highlight remove button with red color
	if (removeLineClicked) {
		cairo_set_source_rgba(cr, 1.0, 0.3, 0.0, 1.0);
	}
	cairo_set_line_width(cr, 1);
	cairo_rectangle(cr, 434, (height - 33) , 130, 30);
	cairo_stroke(cr);
	/// remove text
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_move_to(cr, 462, (height - 12));
	cairo_show_text(cr, "Remove");

	/// clean resources (15 lines fit in the window)
	cairo_surface_flush(surface); // Flush changes to the surface
	cairo_surface_destroy(surface);
	cairo_destroy(cr);

	wl_shm_pool_destroy(pool);
	close(fd);

	munmap(data, size);
	wl_buffer_add_listener(buffer, &wl_buffer_listener_popup, NULL);
	return buffer;
}

/************************************************************************************************/
/*************************************** DRAWING PANEL ******************************************/
/************************************************************************************************/
static void wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
	/// Sent by the compositor when it's no longer using this buffer
	(void)data;
	wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
	.release = wl_buffer_release,
};

static struct wl_buffer *draw_frame(struct client_state *state) {
	/// this function runs in a loop
	int width = state->width;
	int height = state->height;
	int stride = width * 4;
	int size = stride * height;
	int fd = allocate_shm_file(size);

	uint32_t *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
																	WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);

    /// don't draw when the panel is minimized (height is panel height)
    if (height > 1) {
		// cairo drawing
		cairo_surface_t *cairoSurface = cairo_image_surface_create_for_data((unsigned char *)data,
																		CAIRO_FORMAT_ARGB32,
																		width,
																		height,
																		stride);
		cairo_t *cr = cairo_create(cairoSurface);
		cairo_paint(cr);
	
		/// drawing frame aroung the window
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				data[y * width + x] = 0x3b3d40;
			}
		}

		int xPoint = width / 2 - 15;
		int rectHeight = height - 7;

		/********************************** sound **********************************/
		const RsvgRectangle rectSound = {
			.x = xPoint - 13,
			.y = 6,
			.width = width,
			.height = rectHeight - 5,
		};
		/// changing volume icon according to volume level
		currVolLevel = get_current_volume_level();
		if (currVolLevel >= 65) {
			svgSound = rsvg_handle_new_from_file(volumeHigh, NULL);
		}
		else if (currVolLevel < 65 && currVolLevel > 25) {
			svgSound = rsvg_handle_new_from_file(volumeMid, NULL);
		}
		else if (currVolLevel <= 25 && currVolLevel > 2) {
			svgSound = rsvg_handle_new_from_file(volumeLow, NULL);
		}
		else {
			svgSound = rsvg_handle_new_from_file(volumeOff, NULL);
		}
		rsvg_handle_render_document(svgSound, cr, &rectSound, NULL);

		/********************************** brightness **********************************/
		const RsvgRectangle rectBrightness = {
			.x = xPoint - 55,
			.y = 6,
			.width = width,
			.height = rectHeight - 5,
		};

		svgBrightness = rsvg_handle_new_from_file(brightnessIcon, NULL);
		rsvg_handle_render_document(svgBrightness, cr, &rectBrightness, NULL);

		/************************************** time ************************************/
		double fontSize = 20.0;
		const char *currentTime = getcurrenttime();
		
		/// calculate text placement
		int strLen = strlen(currentTime);
		int middleOfBuffWidth = width / 2;
		int startPoint = middleOfBuffWidth - (strLen - fontSize - 29);
		int endPoint = (height + fontSize) / 2.3;

		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
		cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr, fontSize);
		cairo_move_to(cr, startPoint - 45, endPoint);
		cairo_show_text(cr, currentTime);
		
		/********************************* keyboard layout ******************************/
		startPoint = middleOfBuffWidth - (strLen - fontSize) - 5;
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
		cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr, fontSize);
		cairo_move_to(cr, startPoint - 52, endPoint);
		///cairo_show_text(cr, "te");
		/// getting the layout name, first two letters
		if (state->xkb_keymap && keyboard_indicator) {
			const char *kbdLayoutNameFull = xkb_keymap_layout_get_name(state->xkb_keymap,
																				state->xkb_group);
			// Fixing the annoying crash on NULL layout
			if (kbdLayoutNameFull == NULL) {
				kbdLayoutNameFull = "English";
			}
			char kbdLayoutNameСut[strlen(kbdLayoutNameFull) + 3];
			snprintf(kbdLayoutNameСut, sizeof(kbdLayoutNameСut), "%s", kbdLayoutNameFull);
			
			/// check if the first letter is uppercase
			if (kbdLayoutNameСut[0] >= 65 && kbdLayoutNameСut[0] <= 97) {
				kbdLayoutNameСut[0] = kbdLayoutNameСut[0] + 32;
				kbdLayoutNameСut[2] = '\0'; /// cut after second character
			}
			kbdLayoutName = kbdLayoutNameСut;
			cairo_show_text(cr, kbdLayoutName);
		}
		else {
			cairo_show_text(cr, "[!]");
		}

		/*************************************** notes **********************************/
		const RsvgRectangle rectNotes = {
			.x = xPoint - xPoint - 71,
			.y = 6,
			.width = width,
			.height = rectHeight - 5,
		};
		svgNotes = rsvg_handle_new_from_file(notesIcon, NULL);
		rsvg_handle_render_document(svgNotes, cr, &rectNotes, NULL);

		/************************************** network *********************************/
		const RsvgRectangle rectNetwork = {
			.x = xPoint - xPoint - 117,
			.y = 6,
			.width = width,
			.height = rectHeight - 5,
		};
		/// getting current connection state
		char *connectionStateCMD = get_char_value_from_conf(config, "check_connectivity");
		const char *connectionState = output_to_char((const char *)connectionStateCMD);
		if (strcmp(connectionState, "full") == 0) {
			svgNetwork = rsvg_handle_new_from_file(networkIcon, NULL);
		}
		else {
			svgNetwork = rsvg_handle_new_from_file(networkIconOff, NULL);
		}
		rsvg_handle_render_document(svgNetwork, cr, &rectNetwork, NULL);

		/// destroy cairo
		free((void *)currentTime);
		free((void *)connectionState);
		free(connectionStateCMD);
		g_object_unref(svgSound);
		g_object_unref(svgBrightness);
		g_object_unref(svgNotes);
		g_object_unref(svgNetwork);
		cairo_surface_destroy (cairoSurface);
		cairo_destroy (cr);
	}

	munmap(data, size);
	/// very important to release the buffer to prevent memory leak
	wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
	return buffer;
}

static void wl_surface_frame_done(void *data, struct wl_callback *cb, uint32_t time) {
	(void)time;
	/// destroy the previous callback
	wl_callback_destroy(cb);
	/// Request another frame
	struct client_state *state = data;
	cb = wl_display_sync(state->wl_display);
    if (state->height > 1) {
		usleep(100000);
		wl_callback_add_listener(cb, &wl_surface_frame_listener, state);
		/// Submit a frame for this event
		struct wl_buffer *buffer = draw_frame(state);
		wl_surface_attach(state->wl_surface, buffer, 0, 0);
		wl_surface_damage_buffer(state->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
		wl_surface_commit(state->wl_surface);
	}
	else {
		wl_callback_destroy(cb);
		wl_surface_commit(state->wl_surface);
	}
}

static const struct wl_callback_listener wl_surface_frame_listener = {
	.done = wl_surface_frame_done,
};

static void wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities) {
	(void)wl_seat;
	struct client_state *state = data;
	bool have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
	if (have_pointer && state->wl_pointer == NULL) {
		state->wl_pointer = wl_seat_get_pointer(state->wl_seat);
		wl_pointer_add_listener(state->wl_pointer, &wl_pointer_listener, state);
	}
	else if (!have_pointer && state->wl_pointer != NULL) {
		wl_pointer_release(state->wl_pointer);
		state->wl_pointer = NULL;
	}
	bool have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
	if (have_keyboard && state->wl_keyboard == NULL) {
		state->wl_keyboard = wl_seat_get_keyboard(state->wl_seat);
		wl_keyboard_add_listener(state->wl_keyboard, &wl_keyboard_listener, state);
	}
	else if (!have_keyboard && state->wl_keyboard != NULL) {
		wl_keyboard_release(state->wl_keyboard);
		state->wl_keyboard = NULL;
	}
}

static const struct wl_seat_listener wl_seat_listener = {
	.capabilities = wl_seat_capabilities,
	.name = noop,
};

/* Configuring popup, attaching buffer */
static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
	xdg_serial = serial;
    struct client_state *state = data;
    xdg_surface_ack_configure(xdg_surface, serial);
    /// choosing the proper panel widget popup
    ///struct wl_buffer *buffer;
    if (strcmp(selectedWidget, "volume") == 0) {
    	///fprintf(stderr, "CONFIGURE VOLUME\n");
		struct wl_buffer *buffer = draw_frame_volume_bright_popup(state);
		wl_surface_attach(state->wl_surface_popup, buffer, 0, 0);
		wl_surface_commit(state->wl_surface_popup);
		return;
	}
    else if (strcmp(selectedWidget, "brightness") == 0) {
    	///fprintf(stderr, "CONFIGURE BRIGHTNESS\n");
		struct wl_buffer *buffer = draw_frame_volume_bright_popup(state);
		wl_surface_attach(state->wl_surface_popup, buffer, 0, 0);
		xdg_surface_set_window_geometry(state->xdg_surface, (state->x_popup + 80), state->y_popup,
														state->width_popup, state->height_popup);
		wl_surface_commit(state->wl_surface_popup);
		return;
	}
    else if (strcmp(selectedWidget, "datetime") == 0) {
    	///fprintf(stderr, "CONFIGURE DATETIME\n");
		struct wl_buffer *buffer = draw_frame_datetime_popup(state);
		wl_surface_attach(state->wl_surface_popup, buffer, 0, 0);
		xdg_surface_set_window_geometry(state->xdg_surface, (state->x_popup + 700),
							(state->y_popup + 320), state->width_popup, state->height_popup);
		wl_surface_commit(state->wl_surface_popup);
		return;
	}
    else if (strcmp(selectedWidget, "notes") == 0) {
		struct wl_buffer *buffer = draw_frame_notes(state);
		wl_surface_attach(state->wl_surface_popup, buffer, 0, 0);
		xdg_surface_set_window_geometry(state->xdg_surface, (state->x_popup + 1150),
							(state->y_popup + 720), state->width_popup, state->height_popup);
		wl_surface_commit(state->wl_surface_popup);
		return;
	}
    else if (strcmp(selectedWidget, "network") == 0) {
    	/// getting current wifi state enabled/disabled
		char *wifiStateCMD = get_char_value_from_conf(config, "wifi_state");
		char *getCurrConnectionCMD = get_char_value_from_conf(config, "current_active_connection");
		const char *wifiState = output_to_char((const char *)wifiStateCMD);
		currActiveNetwork = output_to_char(getCurrConnectionCMD);
		if (strcmp(wifiState, "enabled") == 0 && !securedNetwork) {
			wifiEnabled = true;
			ssidsTimer = true;
			async_timer(show_networks, 3);
		}
		else {
			wifiEnabled = false;
			ssidsTimer = false;
		}
		/// check if there is any active connection
		char *connectionStateCMD = get_char_value_from_conf(config, "check_connectivity");
		const char *connectionState = output_to_char((const char *)connectionStateCMD);
		if (strcmp(connectionState, "full") == 0) {
			ssidsTimer = true;
		}
		else {
			ssidsTimer = false;
		}

		/// showing either available networks or password prompt
		if (!securedNetwork) {
			struct wl_buffer *buffer = draw_frame_network(state);
			xdg_surface_set_window_geometry(state->xdg_surface, (state->x_popup + 1200),
																	0, state->width_popup, 420);
			wl_surface_damage_buffer(state_ptr->wl_surface_popup, 0, 0, INT32_MAX, INT32_MAX);
			wl_surface_attach(state->wl_surface_popup, buffer, 0, 0);
			wl_surface_commit(state->wl_surface_popup);
		}

		free((void *)wifiState);
		wifiState = NULL;
		free(wifiStateCMD);
		wifiStateCMD = NULL;
		free((void *)connectionState);
		connectionState = NULL;
		free(connectionStateCMD);
		connectionStateCMD = NULL;
		free(getCurrConnectionCMD);
		getCurrConnectionCMD = NULL;
		return;
	}
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

/* Configuring empty popup */
static void xdg_surface_configure_empty(void *data, struct xdg_surface *xdg_surface,
																		uint32_t serial) {
	xdg_serial_empty = serial;
    struct client_state *state = data;
    xdg_surface_ack_configure(xdg_surface, serial);
    struct wl_buffer *buffer = draw_frame_empty(state);
	wl_surface_attach(state->wl_surface_empty, buffer, 0, 0);
	wl_surface_commit(state->wl_surface_empty);
	return;
}

static const struct xdg_surface_listener xdg_surface_listener_empty = {
    .configure = xdg_surface_configure_empty,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
	(void)data;
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void registry_global(void *data, struct wl_registry *wl_registry, uint32_t name,
													const char *interface, uint32_t version) {

	(void)version;
	struct client_state *state = data;
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		state->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
	}
	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		state->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4);
	}
	else if (strcmp(interface, wl_seat_interface.name) == 0) {
		state->wl_seat = wl_registry_bind(wl_registry, name, &wl_seat_interface, 7);
		wl_seat_add_listener(state->wl_seat, &wl_seat_listener, state);
	}
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, state);
    }
	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		/// Bind Layer Shell interface
		state->layer_shell = wl_registry_bind(wl_registry, name, &zwlr_layer_shell_v1_interface, 1);
	}
	else if (strcmp(interface, zwlr_data_control_manager_v1_interface.name) == 0) {
		state->zwlr_data_control_manager_v1 = wl_registry_bind(wl_registry, name,
													&zwlr_data_control_manager_v1_interface, 1);
	}
}

static void registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {
	(void)data;
	(void)name;
	(void)wl_registry;
	/* This space deliberately left blank */
	wl_registry_destroy(wl_registry);
}

static const struct wl_registry_listener wl_registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};

/// configure zwlr_layer_surface_v1
static void layer_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface,
											uint32_t serial, uint32_t width, uint32_t height) {
	(void)width;
	(void)height;
	struct client_state *state = data;
	zwlr_layer_surface_v1_ack_configure(surface, serial);
	struct wl_buffer *buffer = draw_frame(state);
	wl_surface_attach(state->wl_surface, buffer, 0, 0);
	wl_surface_commit(state->wl_surface);
}

static void zwlr_layer_surface_close(void *data, struct zwlr_layer_surface_v1 *surface) {
	(void)surface;
	struct client_state *state = data;
	state->closed = true;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_configure,
	.closed = zwlr_layer_surface_close,
};

int main(void) {
	/* generate configs */
	create_configs();

	/// setting up the global config file path
	const char *HOME = getenv("HOME");
	const char *configDirPath = "/.config/diowpanel";
	const char *configPath = "/.config/diowpanel/diowpanel.conf";
	const char *notesTXTPathB = "/.config/diowpanel/notes.txt";
	const char *iconCachePath = "/.config/diowpanel/icons.cache";
	config = malloc(sizeof(char) * strlen(HOME) + strlen(configPath) + 3);
	configDir = malloc(sizeof(char) * strlen(HOME) + strlen(configDirPath) + 3);
	notesTXTPath = malloc(sizeof(char) * strlen(HOME) + strlen(notesTXTPathB) + 3);
	cacheFilePath = malloc(sizeof(char) * strlen(HOME) + strlen(iconCachePath) + 3);
	snprintf(config, strlen(HOME) + strlen(configPath) + 3, "%s%s", HOME, configPath);
	snprintf(configDir, strlen(HOME) + strlen(configDirPath) + 3, "%s%s", HOME, configDirPath);
	snprintf(notesTXTPath, strlen(HOME) + strlen(notesTXTPathB) + 3, "%s%s", HOME, notesTXTPathB);
	snprintf(cacheFilePath, strlen(HOME) + strlen(iconCachePath) + 3, "%s%s", HOME, iconCachePath);
	/// getting icons
	volumeHigh = malloc(sizeof(char) * strlen(config) + strlen("dio-volume-high.svg") + 3);
	snprintf(volumeHigh, strlen(config) + strlen("dio-volume-high.svg") + 3, "%s/%s",
																configDir, "dio-volume-high.svg");
	volumeMid = malloc(sizeof(char) * strlen(config) + strlen("dio-volume-mid.svg") + 3);
	snprintf(volumeMid, strlen(config) + strlen("dio-volume-mid.svg") + 3, "%s/%s",
																configDir, "dio-volume-mid.svg");
	volumeLow = malloc(sizeof(char) * strlen(config) + strlen("dio-volume-low.svg") + 3);
	snprintf(volumeLow, strlen(config) + strlen("dio-volume-low.svg") + 3, "%s/%s",
																configDir, "dio-volume-low.svg");
	volumeOff = malloc(sizeof(char) * strlen(config) + strlen("dio-volume-off.svg") + 3);
	snprintf(volumeOff, strlen(config) + strlen("dio-volume-off.svg") + 3, "%s/%s",
																configDir, "dio-volume-off.svg");
	brightnessIcon = malloc(sizeof(char) * strlen(config) + strlen("brightness.svg") + 3);
	snprintf(brightnessIcon, strlen(config) + strlen("brightness.svg") + 3, "%s/%s",
																configDir, "brightness.svg");
	notesIcon = malloc(sizeof(char) * strlen(config) + strlen("notes.svg") + 3);
	snprintf(notesIcon, strlen(config) + strlen("notes.svg") + 3, "%s/%s",
																configDir, "notes.svg");
	networkIcon = malloc(sizeof(char) * strlen(config) + strlen("network.svg") + 3);
	snprintf(networkIcon, strlen(config) + strlen("network.svg") + 3, "%s/%s",
																configDir, "network.svg");
	networkIconOff = malloc(sizeof(char) * strlen(config) + strlen("network-off.svg") + 3);
	snprintf(networkIconOff, strlen(config) + strlen("network-off.svg") + 3, "%s/%s",
																configDir, "network-off.svg");
	char *kbdIndicator = get_char_value_from_conf(config, "keyboard_indicator");
	if (strcmp(kbdIndicator, "true") == 0) {
		keyboard_indicator = true;
	}

	struct client_state state = { 0 };
	state_ptr = &state;
	state.width = 300;
	state.height = 40;
	state.width_popup = 30;
	state.height_popup = 170;
	state.x_popup = get_int_value_from_conf(config, "volume_position_x");
	state.y_popup = get_int_value_from_conf(config, "volume_position_y");
    
	state.wl_display = wl_display_connect(NULL);
	state.wl_registry = wl_display_get_registry(state.wl_display);
	wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
    state.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	wl_display_roundtrip(state.wl_display);
	/// set cursor visible on surface
	state.wl_cursor_theme = wl_cursor_theme_load(NULL, 24, state.wl_shm);
	state.wl_cursor = wl_cursor_theme_get_cursor(state.wl_cursor_theme, "left_ptr");
	state.wl_cursor_image = state.wl_cursor->images[0];
	state.wl_cursor_buffer = wl_cursor_image_get_buffer(state.wl_cursor_image);
	state.wl_cursor_surface = wl_compositor_create_surface(state.wl_compositor);
	wl_surface_attach(state.wl_cursor_surface, state.wl_cursor_buffer, 0, 0);
	wl_surface_commit(state.wl_cursor_surface);
	/// end of cursor setup
	state.zwlr_data_control_device_v1 = zwlr_data_control_manager_v1_get_data_device(
											state.zwlr_data_control_manager_v1, state.wl_seat);
	zwlr_data_control_device_v1_add_listener(state.zwlr_data_control_device_v1,
													&zwlr_data_control_device_v1_listener, &state);
	state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
	state.wl_surface_popup = wl_compositor_create_surface(state.wl_compositor);
	state.wl_surface_empty = wl_compositor_create_surface(state.wl_compositor);
    state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface_popup);
    state.xdg_surface_empty = xdg_wm_base_get_xdg_surface(state.xdg_wm_base,
    																	state.wl_surface_empty);
	xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);
	xdg_surface_add_listener(state.xdg_surface_empty, &xdg_surface_listener_empty, &state);
	state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
	state.xdg_toplevel_empty = xdg_surface_get_toplevel(state.xdg_surface_empty);
    xdg_toplevel_set_app_id(state.xdg_toplevel, "diowpanelpopup");
    xdg_toplevel_set_title(state.xdg_toplevel, "Dio Popup");
	xdg_surface_set_window_geometry(state.xdg_surface, state.x_popup, state.y_popup,
																	state.width_popup,
																	state.height_popup);
	state.layer_surface = zwlr_layer_shell_v1_get_layer_surface(state.layer_shell,
																state.wl_surface,
																NULL,
																ZWLR_LAYER_SHELL_V1_LAYER_TOP,
																"diowpanel");
	zwlr_layer_surface_v1_add_listener(state.layer_surface, &layer_surface_listener, &state);
	zwlr_layer_surface_v1_set_size(state.layer_surface, state.width, state.height);
	zwlr_layer_surface_v1_set_anchor(state.layer_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | \
															ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
	/// committing the surface showing the panel
	wl_surface_commit(state.wl_surface);

	pointer_callback = wl_display_sync(state.wl_display);
	wl_callback_add_listener(pointer_callback, &wl_surface_frame_listener, &state);

	while (!state.closed && wl_display_dispatch(state.wl_display) != -1) {
		/* This space deliberately left blank */
	}

	// free resources
	free(config);
	config = NULL;
	free(configDir);
	configDir = NULL;
	free(notesTXTPath);
	notesTXTPath = NULL;
	free(volumeHigh);
	volumeHigh = NULL;
	free(volumeMid);
	volumeMid = NULL;
	free(volumeLow);
	volumeLow = NULL;
	free(volumeOff);
	volumeOff = NULL;
	free(brightnessIcon);
	brightnessIcon = NULL;
	free(notesIcon);
	notesIcon = NULL;
	free(networkIcon);
	networkIcon= NULL;
	free((void *)cacheFilePath);
	cacheFilePath = NULL;
	wl_buffer_destroy(state.wl_cursor_buffer);
	wl_surface_destroy(state.wl_cursor_surface);
	xkb_context_unref(state.xkb_context);
	xkb_keymap_unref(state.xkb_keymap);
	xkb_state_unref(state.xkb_state);
	/// clear SSIDs and security status names
	if (network_ptr->ssids[0] != NULL) {
		for (int i = 0; network_ptr->ssids[i] != NULL; i++) {
			free(network_ptr->ssids[i]);
			network_ptr->ssids[i] = NULL;
			free(network_ptr->keys[i]);
			network_ptr->keys[i] = NULL;
		}
	}
	if (currActiveNetwork != NULL) {
		free((void *)currActiveNetwork);
		currActiveNetwork = NULL;
	}
	if (tempTextBuff != NULL) {
		free(tempTextBuff);
		tempTextBuff = NULL;
	}
	if (hiddenPassword != NULL) {
		free(hiddenPassword);
		hiddenPassword = NULL;
	}
	if (clipboardBuffer) {
		free(clipboardBuffer);
		clipboardBuffer = NULL;
	}
	if (state.zwlr_data_control_offer_v1) {
		zwlr_data_control_offer_v1_destroy(state.zwlr_data_control_offer_v1);
	}
	if (state.zwlr_data_control_manager_v1) {
		zwlr_data_control_manager_v1_destroy(state.zwlr_data_control_manager_v1);
	}
	if (state.zwlr_data_control_device_v1) {
		zwlr_data_control_device_v1_destroy(state.zwlr_data_control_device_v1);
	}
	if (state.wl_keyboard) {
		wl_keyboard_destroy(state.wl_keyboard);
	}
	if (state.xdg_toplevel) {
		xdg_toplevel_destroy(state.xdg_toplevel);
	}
	if (state.xdg_surface) {
		xdg_surface_destroy(state.xdg_surface);
	}
	if (state.layer_surface) {
		zwlr_layer_surface_v1_destroy(state.layer_surface);
	}
	if (state.layer_shell) {
		zwlr_layer_shell_v1_destroy(state.layer_shell);
	}
	if (state.wl_surface) {
		wl_surface_destroy(state.wl_surface);
	}
	if (state.wl_seat) {
		wl_seat_destroy(state.wl_seat);
	}
	if (state.wl_registry) {
		wl_registry_destroy(state.wl_registry);
	}
	if (state.wl_display) {
		wl_display_disconnect(state.wl_display);
	}

	printf("Wayland client terminated!\n");

    return 0;
}
