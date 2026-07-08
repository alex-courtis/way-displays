#include <stdbool.h>
#include <string.h>

#include "util-init.h"

#include "cfg/disabled.h"
#include "head.h"
#include "mode.h"

// TODO shorten
struct Head *head_init_name(const char *name) {
	struct Head *head = head_init();
	head->name = strdup(name);
	return head;
}

// TODO shorten
struct Head *head_init_description(const char *description) {
	struct Head *head = head_init();
	head->description = strdup(description);
	return head;
}

// TODO shorten
struct Disabled *disabled_init_name_desc(const char *name_desc) {
	struct Disabled *d = disabled_init();
	d->name_desc = strdup(name_desc);
	return d;
}

struct Mode *mode_init_whr(int32_t width, int32_t height, int32_t refresh_mhz) {
	struct Mode *mode = mode_init();

	mode->width = width;
	mode->height = height;
	mode->refresh_mhz = refresh_mhz;

	return mode;
}

struct Mode *mode_init_h_whr(struct Head* const head, int32_t width, int32_t height, int32_t refresh_mhz) {
	struct Mode *mode = mode_init_whr(width, height, refresh_mhz);

	mode->head = head;

	return mode;
}

struct Mode *mode_init_whr_max(int32_t width, int32_t height, int32_t refresh_mhz) {
	struct Mode *mode = mode_init_whr(width, height, refresh_mhz);

	mode->max = true;

	return mode;
}
