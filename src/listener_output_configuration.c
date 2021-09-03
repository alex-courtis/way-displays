#include "wlr-output-management-unstable-v1.h"

#include "listeners.h"
#include "types.h"

// OutputManager data

static void succeeded(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	fprintf(stderr, "LOC succeeded\n");
	struct OutputManager *output_manager = data;

	output_manager->changes_complete = true;

	zwlr_output_configuration_v1_destroy(zwlr_output_configuration_v1);

	fprintf(stderr, "LOC succeeded serial %d\n", output_manager->serial);
}

static void failed(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	fprintf(stderr, "LOC failed\n");
	struct OutputManager *output_manager = data;

	output_manager->changes_complete = true;

	zwlr_output_configuration_v1_destroy(zwlr_output_configuration_v1);

	fprintf(stderr, "LOC failed %d\n", output_manager->serial);
	fprintf(stderr, "TODO message\n");
	exit(1);
}

static void cancelled(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	fprintf(stderr, "LOC cancelled\n");
	struct OutputManager *output_manager = data;

	output_manager->changes_complete = true;

	zwlr_output_configuration_v1_destroy(zwlr_output_configuration_v1);

	fprintf(stderr, "LOC cancelled %d\n", output_manager->serial);
	fprintf(stderr, "TODO message\n");
	exit(1);
}

static const struct zwlr_output_configuration_v1_listener listener = {
	.succeeded = succeeded,
	.failed = failed,
	.cancelled = cancelled,
};

const struct zwlr_output_configuration_v1_listener *output_configuration_listener() {
	return &listener;
}

