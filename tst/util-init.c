#include <stdbool.h>
#include <string.h>

#include "util-init.h"

#include "head.h"
#include "mode.h"

struct Head *head_init_name(const char *name) {
	struct Head *head = head_init();
	head->name = strdup(name);
	return head;
}

struct Head *head_init_description(const char *description) {
	struct Head *head = head_init();
	head->description = strdup(description);
	return head;
}

struct WlrMode *wlr_mode_init_whr(int32_t width, int32_t height, int32_t refresh_mhz) {
	struct WlrMode *wlr_mode = wlr_mode_init();

	wlr_mode->width = width;
	wlr_mode->height = height;
	wlr_mode->refresh_mhz = refresh_mhz;

	return wlr_mode;
}

struct WlrMode *wlr_mode_init_h_whr(struct Head* const head, int32_t width, int32_t height, int32_t refresh_mhz) {
	struct WlrMode *wlr_mode = wlr_mode_init_whr(width, height, refresh_mhz);

	wlr_mode->head = head;

	return wlr_mode;
}

struct WlrMode *wlr_mode_init_whr_pref(int32_t width, int32_t height, int32_t refresh_mhz) {
	struct WlrMode *wlr_mode = wlr_mode_init_whr(width, height, refresh_mhz);

	wlr_mode->preferred = true;

	return wlr_mode;
}

struct WlrMode *wlr_mode_init_whr_max(int32_t width, int32_t height, int32_t refresh_mhz) {
	struct WlrMode *wlr_mode = wlr_mode_init_whr(width, height, refresh_mhz);

	wlr_mode->max = true;

	return wlr_mode;
}
