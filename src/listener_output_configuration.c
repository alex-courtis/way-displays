#include <sysexits.h>

#include "listeners.h"
#include "types.h"

// OutputManager data

static void succeeded(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	fprintf(stderr, "LOC succeeded\n");
	struct OutputManager *output_manager = data;
	reset_pending_desired(output_manager);

	printf("Success!\n");
	fflush(stdout);

	zwlr_output_configuration_v1_destroy(zwlr_output_configuration_v1);

	fprintf(stderr, "LOC succeeded serial %d\n", output_manager->serial);
}

static void failed(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {

	// not much we can do here and there is no prior art
	fprintf(stderr, "ERROR: output configuration has failed %s:%d, exiting\n", __FILE__, __LINE__);
	exit(EX_SOFTWARE);
}

static void cancelled(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {

	// there seems to be no way to recover from this
	fprintf(stderr, "ERROR: output configuration has been cancelled %s:%d, exiting\n", __FILE__, __LINE__);
	exit(EX_SOFTWARE);
}

static const struct zwlr_output_configuration_v1_listener listener = {
	.succeeded = succeeded,
	.failed = failed,
	.cancelled = cancelled,
};

const struct zwlr_output_configuration_v1_listener *output_configuration_listener() {
	return &listener;
}

