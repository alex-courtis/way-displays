#include "listeners.h"

#include "log.h"
#include "types.h"

// OutputManager data

static void succeeded(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {
	struct OutputManager *output_manager = data;

	reset_pending_desired(output_manager);

	log_info("\nChanges successful\n");

	for (struct SList *i = output_manager->heads; i; i = i->nex) {
		struct Head *head = i->val;
		if (head->zwlr_config_head) {
			zwlr_output_configuration_head_v1_destroy(head->zwlr_config_head);
			head->zwlr_config_head = NULL;
		}
	}
	zwlr_output_configuration_v1_destroy(zwlr_output_configuration_v1);
}

static void failed(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {

	// not much we can do here and there is no prior art
	log_error("\noutput configuration has failed %s:%d, exiting\n", __FILE__, __LINE__);
	exit(EXIT_FAILURE);
}

static void cancelled(void *data,
		struct zwlr_output_configuration_v1 *zwlr_output_configuration_v1) {

	// there seems to be no way to recover from this
	log_error("\noutput configuration has been cancelled %s:%d, exiting\n", __FILE__, __LINE__);
	exit(EXIT_FAILURE);
}

static const struct zwlr_output_configuration_v1_listener listener = {
	.succeeded = succeeded,
	.failed = failed,
	.cancelled = cancelled,
};

const struct zwlr_output_configuration_v1_listener *output_configuration_listener() {
	return &listener;
}

