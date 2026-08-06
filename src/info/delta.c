#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>

#include "info/delta.h"

#include "cfg/condition.h"
#include "enum.h"
#include "head.h"
#include "mode.h"
#include "ppmap.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"

char *delta_human(const struct PPmap * const heads) {
	if (!heads) {
		return NULL;
	}

	char *delta = NULL;

	for (const struct PPmapIt *it = ppmap_it(heads); it; it = ppmap_it_next(it)) {
		const struct Head * head = it->val;

		// disable in own operation
		if (head->cur.enabled && !head->des.enabled) {
			if (head->disabled_condition_met) {
				char *msg = cfg_condition_str(head->disabled_condition_met);
				delta = sprintf_append(delta, "%s\n  disabled: %s\n", head_human(head), msg);
				free(msg);
			} else {
				delta = sprintf_append(delta, "%s\n  disabled\n", head_human(head));
			}
			continue;
		}

		// enable in own operation
		if (!head->cur.enabled && head->des.enabled) {
			delta = sprintf_append(delta, "%s\n  enabled\n", head_human(head));
			continue;
		}

		if (head_current_not_desired(head)) {
			delta = sprintf_append(delta, "%s\n", head_human(head));

			if (head->cur.scale != head->des.scale) {
				delta = sprintf_append(delta, "  scale:     %.3f -> %.3f\n",
						wl_fixed_to_double(head->cur.scale),
						wl_fixed_to_double(head->des.scale)
						);
			}

			if (head->cur.transform != head->des.transform) {
				delta = sprintf_append(delta, "  transform: %s -> %s\n",
						head->cur.transform ? transform_name(head->cur.transform) : "none",
						head->des.transform ? transform_name(head->des.transform) : "none"
						);
			}

			if (head->cur.x != head->des.x || head->cur.y != head->des.y) {
				delta = sprintf_append(delta, "  position:  %d,%d -> %d,%d\n",
						head->cur.x, head->cur.y,
						head->des.x, head->des.y
						);
			}
		}
	}

	// strip trailing newline
	if (delta) {
		size_t len = strlen(delta);
		if (len > 0) {
			delta[len - 1] = '\0';
		}
	}

	return delta;
}

char *delta_human_mode(const struct Head * const head) {
	if (!head) {
		return NULL;
	}

	char *delta = NULL;

	delta = sprintf_append(delta, "%s\n  ",
			head_human(head)
			);

	const struct Mode *mode_cur = ppmap_get(head->modes, head->cur.zmode);
	if (mode_cur) {
		delta = sprintf_append(delta, "%dx%d@%dHz -> ",
				mode_cur->width,
				mode_cur->height,
				mode_hz_rounded(mode_cur)
				);
	} else {
		delta = sprintf_append(delta, "(no mode) -> ");
	}

	const struct Mode *mode_des = ppmap_get(head->modes, head->des.zmode);
	if (mode_des) {
		delta = sprintf_append(delta, "%dx%d@%dHz",
				mode_des->width,
				mode_des->height,
				mode_hz_rounded(mode_des)
				);
	} else {
		delta = sprintf_append(delta, "(no mode)");
	}

	return delta;
}


char *delta_human_adaptive_sync(const struct Head * const head) {
	if (!head) {
		return NULL;
	}

	return sprintf_append(NULL, "%s\n  VRR %s",
			head_human(head),
			head->des.adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED ? "on" : "off"
			);
}

char *delta_human_reapply(const struct Head * const head) {
	if (!head)
		return NULL;

	return sprintf_alloc("%s\n  disabled\n  modes reset",
			head_human(head)
			);
}

