#ifndef LISTENERS_H
#define LISTENERS_H

#define ZWLR_OUTPUT_MANAGER_V1_VERSION 4
#define ZWLR_OUTPUT_MANAGER_V1_VERSION_MIN 2

// data
const struct wl_registry_listener *registry_listener(void);
const struct zwlr_output_manager_v1_listener *output_manager_listener(void);
const struct zwlr_output_head_v1_listener *head_listener(void);
const struct zwlr_output_head_v1_listener *head_listener_min(void);
const struct zwlr_output_mode_v1_listener *mode_listener(void);

// config
const struct zwlr_output_configuration_v1_listener *output_configuration_listener(void);

#endif // LISTENERS_H

