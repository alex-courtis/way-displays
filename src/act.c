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
				head->cur.zmode = g_displ->delta.zmode;
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

	switch(g_displ->delta.element) {
		case MODE:
			if (head) {
				const struct zwlr_output_mode_v1 *zmode = g_displ->delta.zmode;

				print_mode_fail(ERROR, head, zmode);
				callback_mode_fail(ERROR, head, zmode);

				// move the mode to failed
				ppmap_put(head->modes_failed, zmode, ppmap_remove(head->modes, zmode));

				// current mode may be misreported
				head->cur.zmode = NULL;
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

	displ_delta_destroy(g_displ);
}

void act_apply(void) {
	const struct PPmap *heads_changing = head_ppmap_init();

	displ_delta_destroy(g_displ);

	struct PPmapFilter f = { 0 };

	// determine whether changes are needed before initiating output configuration
	f.val = (fn_pred_p)head_current_not_desired;
	for (const struct PPmapIt *it = ppmap_filter_it(g_displ->heads, f); it; it = ppmap_it_next(it)) {
		ppmap_put(heads_changing, it->key, it->val);
	}

	if (ppmap_size(heads_changing) == 0) {
		ppmap_free(heads_changing);
		return;
	}

	// create and start the listener
	struct zwlr_output_configuration_v1 *zconfig = create_zwlr_output_config_listener(g_displ);

	struct PPmapPair p;
	struct Head *head;

	// 1 - reapply
	f.val = (fn_pred_p)head_reapply_required;
	p = ppmap_find(heads_changing, f);
	if (p.val) {
		head = (struct Head*)p.val;

		displ_delta_init(g_displ, 0, head, NULL);

		print_head(INFO, DELTA, head);

		_zwlr_output_configuration_v1_disable_head(zconfig, (struct zwlr_output_head_v1*)p.key);

		g_displ->delta.human = delta_human_reapply(head);

		head->reapply_required = false;

		goto apply;
	}

	// 2 - single mode
	f.val = (fn_pred_p)head_current_mode_not_desired;
	p = ppmap_find(heads_changing, f);
	if (p.val) {
		head = (struct Head*)p.val;

		displ_delta_init(g_displ, MODE, head, head->des.zmode);

		print_head(INFO, DELTA, head);

		// mode change in its own operation; mode change desire is always enabled
		head->zconfig = _zwlr_output_configuration_v1_enable_head(zconfig, (struct zwlr_output_head_v1*)p.key);
		_zwlr_output_configuration_head_v1_set_mode(head->zconfig, (struct zwlr_output_mode_v1*)head->des.zmode);

		g_displ->delta.human = delta_human_mode(head);

		goto apply;
	}

	// 3 - single VRR
	f.val = (fn_pred_p)head_current_adaptive_sync_not_desired;
	p = ppmap_find(heads_changing, f);
	if (p.val) {
		head = (struct Head*)p.val;

		displ_delta_init(g_displ, VRR_OFF, head, NULL);

		print_head(INFO, DELTA, head);

		// adaptive sync change in its own operation; adaptive sync change desire is always enabled
		head->zconfig = _zwlr_output_configuration_v1_enable_head(zconfig, (struct zwlr_output_head_v1*)p.key);
		_zwlr_output_configuration_head_v1_set_adaptive_sync(head->zconfig, head->des.adaptive_sync);

		g_displ->delta.human = delta_human_adaptive_sync(head);

		goto apply;
	}

	// otherwise apply everything else
	{
		displ_delta_init(g_displ, 0, NULL, NULL);

		print_head_map(INFO, DELTA, heads_changing);

		// all other changes
		for (const struct PPmapIt *it = ppmap_it(heads_changing); it; it = ppmap_it_next(it)) {
			head = (struct Head*)it->val;

			if (head->des.enabled) {
				head->zconfig = _zwlr_output_configuration_v1_enable_head(zconfig, (void*)it->key);
				_zwlr_output_configuration_head_v1_set_scale(head->zconfig, head->des.scale);
				_zwlr_output_configuration_head_v1_set_position(head->zconfig, head->des.x, head->des.y);
				_zwlr_output_configuration_head_v1_set_transform(head->zconfig, head->des.transform);
			} else {
				_zwlr_output_configuration_v1_disable_head(zconfig, (void*)it->key);
			}
		}

		g_displ->delta.human = delta_human(heads_changing);
	}

apply:
	_zwlr_output_configuration_v1_apply(zconfig);

	g_displ->state = OUTSTANDING;

	ppmap_free(heads_changing);
}

void act(void) {
	print_head_set(INFO, ARRIVED, g_displ->heads_arrived);
	pset_remove_all(g_displ->heads_arrived);

	print_head_set(INFO, DEPARTED, g_displ->heads_departed);
	pset_remove_all_free(g_displ->heads_departed);

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

