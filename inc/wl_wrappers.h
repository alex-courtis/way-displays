#ifndef WL_WRAPPERS_H
#define WL_WRAPPERS_H

#include <stdint.h>
#include <wayland-client-protocol.h>

#include "displ.h"
#include "wlr-output-management-unstable-v1.h"

// use multiple wrappers to disambiguate stacks
int _wl_display_dispatch_pending__read_events(struct wl_display *display, char *file, int line);
int _wl_display_dispatch_pending__prepare_read(struct wl_display *display, char *file, int line);

int _wl_display_prepare_read(struct wl_display *display, char *file, int line);

int _wl_display_flush(struct wl_display *display, char *file, int line);

// returns 0 on success, -1 on epipe, exits otherwise
int _wl_display_read_events(struct wl_display *display, char *file, int line);

// zwlr_output_manager_v1_create_configuration and zwlr_output_configuration_v1_add_listener
struct zwlr_output_configuration_v1 *create_zwlr_output_config_listener(struct Displ *displ);

struct zwlr_output_configuration_head_v1 *_zwlr_output_configuration_v1_enable_head(struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1, struct zwlr_output_head_v1 *head);

void _zwlr_output_configuration_v1_disable_head(struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1, struct zwlr_output_head_v1 *head);

void _zwlr_output_configuration_v1_apply(struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1);

void _zwlr_output_configuration_head_v1_set_mode(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, struct zwlr_output_mode_v1 *mode);

void _zwlr_output_configuration_head_v1_set_transform(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, int32_t transform);

void _zwlr_output_configuration_head_v1_set_scale(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, int32_t scale);

void _zwlr_output_configuration_head_v1_set_position(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, int32_t x, int32_t y);

void _zwlr_output_configuration_head_v1_set_adaptive_sync(struct zwlr_output_configuration_head_v1 *zwlr_output_configuration_head_v1, uint32_t state);

#endif // WL_WRAPPERS_H

