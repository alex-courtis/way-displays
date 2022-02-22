#ifndef LISTENERS_H
#define LISTENERS_H

// data
const struct wl_registry_listener *registry_listener();
const struct zwlr_output_manager_v1_listener *output_manager_listener();
const struct zwlr_output_head_v1_listener *head_listener();
const struct zwlr_output_mode_v1_listener *mode_listener();

// config
const struct zwlr_output_configuration_v1_listener *output_configuration_listener();

#endif // LISTENERS_H

