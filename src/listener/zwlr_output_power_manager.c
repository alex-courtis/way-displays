#include <stdint.h>

#include "listeners.h"

#include "log.h"
#include "wlr-output-power-management-unstable-v1.h"

// Output data

static void mode(void *data,
		struct zwlr_output_power_v1 *zwlr_output_power_v1,
		uint32_t mode) {

	log_debug("EVENT: zwlr_output_power_v1_listener mode=%u output=%p", mode, data);
}

void failed(void *data,
		struct zwlr_output_power_v1 *zwlr_output_power_v1) {

	log_debug("EVENT: zwlr_output_power_v1_listener failed output=%p", data);
}

static const struct zwlr_output_power_v1_listener listener = {
	.mode = mode,
	.failed = failed,
};

const struct zwlr_output_power_v1_listener *zwlr_output_power_listener(void) {
	return &listener;
}

