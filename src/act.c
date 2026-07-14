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
#include "process.h"
#include "pset.h"
#include "pslist.h"
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
				if (head_current_adaptive_sync_not_desired(head)) {
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

	displ_delta_destroy();
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

	switch(g_displ->delta.element) {
		case MODE:
			if (head) {
				print_mode_fail(ERROR, head, head->desired.mode);
				callback_mode_fail(ERROR, head, head->desired.mode);

				// mode setting failure, try again with another mode
				pset_add(head->modes_failed, head->desired.mode);
				pset_remove(head->modes, head->desired.mode);

				// current mode may be misreported
				head->current.mode = NULL;
			}

			break;

		case VRR_OFF:
			if (head) {
				// river reports adaptive sync failure as failure
				if (head_current_adaptive_sync_not_desired(head)) {

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

	displ_delta_destroy();
}

void act_apply(void) {
	struct Pslist *heads_changing = NULL;

	displ_delta_destroy();

	// determine whether changes are needed before initiating output configuration
	struct Pslist *i = g_heads;
	while ((i = pslist_find(i, (fn_pred)head_current_not_desired))) {
		pslist_append(&heads_changing, i->val);
		i = i->nex;
	}
	if (!heads_changing)
		return;

	// create and start the listener
	struct zwlr_output_configuration_v1 *zwlr_config = create_zwlr_output_config_listener();

	struct Head *head;

	if ((head = pslist_find_val(g_heads, (fn_pred)head_reapply_required))) {
		displ_delta_init(0, head);

		print_head(INFO, DELTA, head);

		_zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);

		g_displ->delta.human = delta_human_reapply(head);

		head->reapply_required = false;

	} else if ((head = pslist_find_val(g_heads, (fn_pred)head_current_mode_not_desired))) {
		displ_delta_init(MODE, head);

		print_head(INFO, DELTA, head);

		// mode change in its own operation; mode change desire is always enabled
		head->zwlr_config_head = _zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
		_zwlr_output_configuration_head_v1_set_mode(head->zwlr_config_head, head->desired.mode->zwlr_mode);

		g_displ->delta.human = delta_human_mode(head);

	} else if ((head = pslist_find_val(g_heads, (fn_pred)head_current_adaptive_sync_not_desired))) {
		displ_delta_init(VRR_OFF, head);

		print_head(INFO, DELTA, head);

		// adaptive sync change in its own operation; adaptive sync change desire is always enabled
		head->zwlr_config_head = _zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
		_zwlr_output_configuration_head_v1_set_adaptive_sync(head->zwlr_config_head, head->desired.adaptive_sync);

		g_displ->delta.human = delta_human_adaptive_sync(head);

	} else {
		displ_delta_init(0, NULL);

		print_heads(INFO, DELTA, g_heads);

		// all other changes
		for (i = heads_changing; i; i = i->nex) {
			head = (struct Head*)i->val;

			if (head->desired.enabled) {
				head->zwlr_config_head = _zwlr_output_configuration_v1_enable_head(zwlr_config, head->zwlr_head);
				_zwlr_output_configuration_head_v1_set_scale(head->zwlr_config_head, head->desired.scale);
				_zwlr_output_configuration_head_v1_set_position(head->zwlr_config_head, head->desired.x, head->desired.y);
				_zwlr_output_configuration_head_v1_set_transform(head->zwlr_config_head, head->desired.transform);
			} else {
				_zwlr_output_configuration_v1_disable_head(zwlr_config, head->zwlr_head);
			}
		}

		g_displ->delta.human = delta_human(heads_changing);
	}

	_zwlr_output_configuration_v1_apply(zwlr_config);

	g_displ->state = OUTSTANDING;

	pslist_free(&heads_changing);
}

void act(void) {
	print_heads(INFO, ARRIVED, g_heads_arrived);
	pslist_free(&g_heads_arrived);

	print_heads(INFO, DEPARTED, g_heads_departed);
	pslist_free_vals(&g_heads_departed, (fn_free)head_free);

	print_head_queue(DEBUG, "act started", g_displ->state, g_heads);

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
	print_head_queue(DEBUG, "act desired", g_displ->state, g_heads);

	act_apply();
	print_head_queue(DEBUG, "act applied", g_displ->state, g_heads);
}

