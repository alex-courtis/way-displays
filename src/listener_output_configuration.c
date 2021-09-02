#include "wlr-output-management-unstable-v1.h"

#include "listeners.h"
#include "types.h"

// OutputManager data

static void succeeded(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	struct OutputManager *output_manager = data;

	output_manager->serial_cfg_done = output_manager->serial;
	fprintf(stderr, "LOC succeeded %d\n", output_manager->serial_cfg_done);
}

static void failed(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	struct OutputManager *output_manager = data;

	// TODO output a message then carry on, not attempting this change again
	output_manager->serial_cfg_done = output_manager->serial;

	fprintf(stderr, "LOC failed %d\n", output_manager->serial_cfg_done);
}

static void cancelled(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	struct OutputManager *output_manager = data;

	output_manager->serial_cfg_done = output_manager->serial;
	fprintf(stderr, "LOC cancelled %d\n", output_manager->serial_cfg_done);
}

static const struct zwlr_output_configuration_v1_listener listener = {
	.succeeded = succeeded,
	.failed = failed,
	.cancelled = cancelled,
};

const struct zwlr_output_configuration_v1_listener *output_configuration_listener() {
	return &listener;
}

