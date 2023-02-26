#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include <stdint.h>

#include "cfg.h"
#include "list.h"

struct Mode {
	struct Head *head;

	struct zwlr_output_mode_v1 *zwlr_mode;

	int32_t width;
	int32_t height;
	int32_t refresh_mhz;
	bool preferred;
};

struct ModesResRefresh {
	int32_t width;
	int32_t height;
	int32_t refresh_hz;
	struct SList *modes;
};

int32_t mhz_to_hz(int32_t mhz);

double mode_dpi(struct Mode *mode);

struct SList *modes_res_refresh(struct SList *modes);

bool mrr_satisfies_user_mode(struct ModesResRefresh *mrr, struct UserMode *user_mode);

void mode_free(void *mode);

void mode_res_refresh_free(void *mode);

struct Mode *mode_user_mode(struct SList *modes, struct SList *modes_failed, struct UserMode *user_mode);

#endif // MODE_H

