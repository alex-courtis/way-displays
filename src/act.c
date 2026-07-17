#include <stdbool.h>
#include <stdlib.h>

#include "act.h"

#include "desire.h"
#include "displ.h"
#include "enum.h"
#include "fn.h"
#include "head.h"
#include "info/callback.h"
#include "info/delta.h"
#include "info/print.h"
#include "log.h"
#include "mode.h"
#include "ppmap.h"
#include "process.h"
#include "pset.h"
#include "str.h"
#include "wl_wrappers.h"

#define MAX_CANCELLATION_RETRIES 5

int g_cancellation_retries = 0;

void act_handle_success(void) {
	g_cancellation_retries = 0;

	struct Head *head = g_displ->delta.head;

	if (head) {
		switch(g_displ->delta.element) {
			case MODE:
				// successful mode change is not always reported
				head->current.mode = head->desired.mode;
				break;

			case VRR_OFF:
				// sway reports adaptive sync failure as success
				if (head_current_adaptive_sync_not_desired(head, NULL)) {
					act_handle_failure();
					return;
				}
				break;

			default:
				break;
		}
	}

	log_info(NULL);
	log_info("Changes successful");
	callback(INFO, g_displ->delta.human ? g_displ->delta.human : "Changes successful", NULL);

	displ_delta_destroy(g_displ);
}

bool act_handle_cancelled(void) {
	char *msg;
	bool ret = false;

	if (++g_cancellation_retries <= MAX_CANCELLATION_RETRIES) {
		msg = sprintf_alloc("Changes cancelled, retrying (attempt %i)", g_cancellation_retries);
		ret = true;
	} else {
		msg = sprintf_alloc("Changes cancelled after %i retries", MAX_CANCELLATION_RETRIES);
		ret = false;
	}

	log_warn(NULL);
	log_warn("%s", msg);
	callback(WARNING, msg, NULL);

	free(msg);

	return ret;
}

void act_handle_failure(void) {
	struct Head *head = g_displ->delta.head;

	// TODO consider putting data in delta, for zwlr_mode
	switch(g_displ->delta.element) {
		case MODE:
			if (head) {
				print_mode_fail(ERROR, head, head->desired.mode);
				callback_mode_fail(ERROR, head, head->desired.mode);

				const struct Mode *mode_failed = head->desired.mode;

				// mode setting failure, try again with another mode
				if (mode_failed) {
					ppmap_remove(head->modes, mode_failed->zwlr_mode);
					ppmap_put(head->modes_failed, mode_failed->zwlr_mode, mode_failed);
				}

				// current mode may be misreported
				head->current.mode = NULL;
			}

			break;

		case VRR_OFF:
			if (head) {
				// river reports adaptive sync failure as failure
				if (head_current_adaptive_sync_not_desired(head, NULL)) {

					print_adaptive_sync_fail(WARNING, head);
					callback_adaptive_sync_fail(WARNING, head);

					head->adaptive_sync_failed = true;
				}
			}

			break;
		default:
			log_fatal(NULL);
			log_fatal("Changes failed, exiting");
			callback(FATAL, g_displ->delta.human, "\nChanges failed, exiting");

			wd_exit_message(EXIT_FAILURE);
			break;
	}

	displ_delta_destroy(g_displ);
}

void act_apply(void) {
	const struct PPmap *heads_changing = head_ppmap_init();

	displ_delta_destroy(g_displ);

	// determine whether changes are needed before initiating output configuration
	for (const struct PPmapIt *it = ppmap_val_filter_it(g_displ->heads, (fn_2pred)head_current_not_desired, NULL); it; it = ppmap_it_next(it)) {
		ppmap_put(heads_changing, it->key, it->val);
	}

	if (ppmap_size(heads_changing) == 0) {
		ppmap_free(heads_changing);
		return;
	}

	// create and start the listener
	struct zwlr_output_configuration_v1 *zwlr_config = create_zwlr_output_config_listener(g_displ);

	struct PPmapPair pair;
	struct Head *head;

	// 1 - reapply
	pair = ppmap_find_val(heads_changing, (fn_2pred)head_reapply_required, NULL);
	if (pair.val) {
		head = (struct Head*)pair.val;

		displ_delta_init(g_displ, 0, head);

		print_head(INFO, DELTA, head);

		_zwlr_output_configuration_v1_disable_head(zwlr_config, (struct zwlr_output_head_v1*)pair.key);

		g_displ->delta.human = delta_human_reapply(head);

		head->reapply_required = false;

		goto apply;
	}

	// 2 - single mode
	pair = ppmap_find_val(heads_changing, (fn_2pred)head_current_mode_not_desired, NULL);
	if (pair.val) {
		head = (struct Head*)pair.val;

		displ_delta_init(g_displ, MODE, head);

		print_head(INFO, DELTA, head);

		// mode change in its own operation; mode change desire is always enabled
		head->zwlr_config_head = _zwlr_output_configuration_v1_enable_head(zwlr_config, (struct zwlr_output_head_v1*)pair.key);
		_zwlr_output_configuration_head_v1_set_mode(head->zwlr_config_head, head->desired.mode->zwlr_mode);

		g_displ->delta.human = delta_human_mode(head);

		goto apply;
	}

	// 3 - single VRR
	pair = ppmap_find_val(heads_changing, (fn_2pred)head_current_adaptive_sync_not_desired, NULL);
	if (pair.val) {
		head = (struct Head*)pair.val;

		displ_delta_init(g_displ, VRR_OFF, head);

		print_head(INFO, DELTA, head);

		// adaptive sync change in its own operation; adaptive sync change desire is always enabled
		head->zwlr_config_head = _zwlr_output_configuration_v1_enable_head(zwlr_config, (struct zwlr_output_head_v1*)pair.key);
		_zwlr_output_configuration_head_v1_set_adaptive_sync(head->zwlr_config_head, head->desired.adaptive_sync);

		g_displ->delta.human = delta_human_adaptive_sync(head);

		goto apply;
	}

	// otherwise apply everything else
	{
		displ_delta_init(g_displ, 0, NULL);

		print_head_map(INFO, DELTA, heads_changing);

		// all other changes
		for (const struct PPmapIt *it = ppmap_it(heads_changing); it; it = ppmap_it_next(it)) {
			head = (struct Head*)it->val;

			if (head->desired.enabled) {
				head->zwlr_config_head = _zwlr_output_configuration_v1_enable_head(zwlr_config, (void*)it->key);
				_zwlr_output_configuration_head_v1_set_scale(head->zwlr_config_head, head->desired.scale);
				_zwlr_output_configuration_head_v1_set_position(head->zwlr_config_head, head->desired.x, head->desired.y);
				_zwlr_output_configuration_head_v1_set_transform(head->zwlr_config_head, head->desired.transform);
			} else {
				_zwlr_output_configuration_v1_disable_head(zwlr_config, (void*)it->key);
			}
		}

		g_displ->delta.human = delta_human(heads_changing);
	}

apply:
	_zwlr_output_configuration_v1_apply(zwlr_config);

	g_displ->state = OUTSTANDING;

	ppmap_free(heads_changing);
}

void act(void) {
	print_head_set(INFO, ARRIVED, g_displ->heads_arrived);
	// TODO need a clear collection
	pset_free(g_displ->heads_arrived);
	g_displ->heads_arrived = head_pset_init();

	print_head_set(INFO, DEPARTED, g_displ->heads_departed);
	pset_free_vals(g_displ->heads_departed);
	g_displ->heads_departed = head_pset_init();

	print_head_queue(FATAL, g_displ, "act started");

	switch (g_displ->state) {
		case SUCCEEDED:
			act_handle_success();
			g_displ->state = IDLE;
			break;

		case OUTSTANDING:
			// wait
			return;

		case FAILED:
			act_handle_failure();
			g_displ->state = IDLE;
			break;

		case CANCELLED:
			g_displ->state = IDLE;
			// whether to keep retrying
			if (act_handle_cancelled()) {
				break;
			} else {
				return;
			}

		case IDLE:
		default:
			break;
	}

	desire();
	print_head_queue(FATAL, g_displ, "act desired");

	act_apply();
	print_head_queue(FATAL, g_displ, "act applied");
}

