#include <stdbool.h>
#include <string.h>

#include "util-init.h"

#include "cfg/disabled.h"
#include "head.h"
#include "mode.h"

struct Head *head_n(const char *name) {
	struct Head *head = head_init();
	head->name = strdup(name);
	return head;
}

struct Head *head_d(const char *description) {
	struct Head *head = head_init();
	head->description = strdup(description);
	return head;
}

struct CfgDisabled *disabled_nd(const char *name_desc) {
	struct CfgDisabled *d = cfg_disabled_init();
	d->name_desc = strdup(name_desc);
	return d;
}

struct Mode *mode_whr(int32_t width, int32_t height, int32_t refresh_mhz) {
	struct Mode *mode = mode_init();

	mode->width = width;
	mode->height = height;
	mode->refresh_mhz = refresh_mhz;

	return mode;
}

struct Mode *mode_h(struct Head* const head) {
	struct Mode *mode = mode_init();

	mode->head = head;

	return mode;
}

struct Mode *mode_h_whr(struct Head* const head, int32_t width, int32_t height, int32_t refresh_mhz) {
	struct Mode *mode = mode_whr(width, height, refresh_mhz);

	mode->head = head;

	return mode;
}

struct Mode *mode_whr_max(int32_t width, int32_t height, int32_t refresh_mhz) {
	struct Mode *mode = mode_whr(width, height, refresh_mhz);

	mode->max = true;

	return mode;
}
