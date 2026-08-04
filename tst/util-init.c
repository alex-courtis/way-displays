#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "util-init.h"

#include "head.h"
#include "mode.h"
#include "output.h"

struct Head *head_n(const char *name) {
	struct Head *head = head_init();
	head->name = strdup(name);
	return head;
}

struct Head *head_n_en(const char *name, bool enabled) {
	struct Head *head = head_init();
	head->name = strdup(name);
	head->cur.enabled = enabled;
	return head;
}

struct Head *head_d(const char *description) {
	struct Head *head = head_init();
	head->description = strdup(description);
	return head;
}

struct Output *output_n(const char *name) {
	struct Output *output = calloc(1, sizeof(struct Output));
	output->name = strdup(name);
	return output;
}

struct Mode *mode_whr(int32_t width, int32_t height, int32_t refresh_mhz) {
	struct Mode *mode = mode_init();

	mode->width = width;
	mode->height = height;
	mode->refresh_mhz = refresh_mhz;

	return mode;
}

struct Mode *mode_whr_max(int32_t width, int32_t height, int32_t refresh_mhz) {
	struct Mode *mode = mode_whr(width, height, refresh_mhz);

	mode->max = true;

	return mode;
}

struct Mode *mode_whr_max_pref(int32_t width, int32_t height, int32_t refresh_mhz) {
	struct Mode *mode = mode_whr(width, height, refresh_mhz);

	mode->max_preferred_refresh = true;

	return mode;
}
