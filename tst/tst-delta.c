#include "tst.h"

#include "asserts.h"
#include "data.h"
#include "util-init.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>

#include "head.h"
#include "ppmap.h"
#include "wlr-output-management-unstable-v1.h"

#include "info/delta.h"

struct State {
	struct Head *head1;
	struct Head *head2;
	const struct PPmap *heads;
};

int before_each(void **state) {
	struct State *s = calloc(1, sizeof(struct State));

	s->heads = head_ppmap_init();

	s->head1 = head_n("name1");
	s->head1->description = strdup("description1");

	s->head1->cur.scale = 512;
	s->head1->cur.enabled = true;
	s->head1->cur.x = 700;
	s->head1->cur.y = 800;
	s->head1->cur.transform = WL_OUTPUT_TRANSFORM_180;
	s->head1->cur.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	ppmap_put(s->head1->modes, M0, mode_whr(100, 200, 30000));
	s->head1->cur.zmode = M0;

	s->head1->des.scale = 1024;
	s->head1->des.enabled = true;
	s->head1->des.x = 900;
	s->head1->des.y = 1000;
	s->head1->des.transform = WL_OUTPUT_TRANSFORM_90;
	s->head1->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	ppmap_put(s->head1->modes, M1, mode_whr(400, 500, 60000));
	s->head1->des.zmode = M1;

	ppmap_put(s->heads, H1, s->head1);


	s->head2 = head_n("name2");

	s->head2->cur.scale = 2048;
	s->head2->cur.enabled = true;
	s->head2->cur.x = 1700;
	s->head2->cur.y = 1800;
	s->head2->cur.transform = WL_OUTPUT_TRANSFORM_270;
	s->head2->cur.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	ppmap_put(s->head2->modes, M0, mode_whr(1100, 1200, 130000));
	s->head2->cur.zmode = M0;

	s->head2->des.scale = 4096;
	s->head2->des.enabled = true;
	s->head2->des.x = 1900;
	s->head2->des.y = 11000;
	s->head2->des.transform = WL_OUTPUT_TRANSFORM_NORMAL;
	s->head2->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	ppmap_put(s->head2->modes, M1, mode_whr(1400, 1500, 160000));
	s->head2->des.zmode = M1;

	ppmap_put(s->heads, H2, s->head2);

	*state = s;
	return 0;
}

int after_each(void **state) {
	struct State *s = *state;

	ppmap_free_vals(s->heads);

	free(s);

	return 0;
}

static void delta_human_mode__to_mode(void **state) {
	const struct State *s = *state;

	char *deltas = delta_human_mode(s->head2);

	assert_str_equal(deltas, ""
			"name2\n"
			"  1100x1200@130Hz -> 1400x1500@160Hz"
			);

	free(deltas);
}

static void delta_human_mode__to_no(void **state) {
	struct State *s = *state;

	s->head1->des.zmode = NULL;

	char *deltas = delta_human_mode(s->head1);

	assert_str_equal(deltas, ""
			"description1\n"
			"  100x200@30Hz -> (no mode)"
			);

	free(deltas);
}

static void delta_human_mode__from_no(void **state) {
	struct State *s = *state;

	s->head2->cur.zmode = NULL;

	char *deltas = delta_human_mode(s->head2);

	assert_str_equal(deltas, ""
			"name2\n"
			"  (no mode) -> 1400x1500@160Hz"
			);

	free(deltas);
}

static void delta_human_adaptive_sync__on(void **state) {
	struct State *s = *state;

	s->head1->cur.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;
	s->head1->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;

	char *deltas = delta_human_adaptive_sync(s->head1);

	assert_str_equal(deltas, ""
			"description1\n"
			"  VRR on"
			);

	free(deltas);
}

static void delta_human_adaptive_sync__off(void **state) {
	struct State *s = *state;

	s->head2->cur.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED;
	s->head2->des.adaptive_sync = ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	char *deltas = delta_human_adaptive_sync(s->head2);

	assert_str_equal(deltas, ""
			"name2\n"
			"  VRR off"
			);

	free(deltas);
}

static void delta_human_reapply__(void **state) {
	const struct State *s = *state;

	char *deltas = delta_human_reapply(s->head2);

	assert_str_equal(deltas, ""
			"name2\n"
			"  disabled\n"
			"  modes reset"
			);

	free(deltas);
}

static void delta_human__all(void **state) {
	const struct State *s = *state;

	char *deltas = delta_human(s->heads);

	assert_str_equal(deltas, ""
			"description1\n"
			"  scale:     2.000 -> 4.000\n"
			"  transform: 180 -> 90\n"
			"  position:  700,800 -> 900,1000\n"
			"name2\n"
			"  scale:     8.000 -> 16.000\n"
			"  transform: 270 -> none\n"
			"  position:  1700,1800 -> 1900,11000"
			);

	free(deltas);
}

static void delta_human__enabled(void **state) {
	struct State *s = *state;

	s->head1->cur.enabled = false;
	s->head1->des.enabled = true;

	s->head2->cur.enabled = false;
	s->head2->des.enabled = true;

	char *deltas = delta_human(s->heads);

	assert_str_equal(deltas, ""
			"description1\n  enabled\n"
			"name2\n  enabled"
			);

	free(deltas);
}

static void delta_human__disabled(void **state) {
	struct State *s = *state;

	s->head1->cur.enabled = true;
	s->head1->des.enabled = false;

	s->head2->cur.enabled = true;
	s->head2->des.enabled = false;

	char *deltas = delta_human(s->heads);

	assert_str_equal(deltas, ""
			"description1\n  disabled\n"
			"name2\n  disabled"
			);

	free(deltas);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST_BA(delta_human_mode__to_mode),
		TEST_BA(delta_human_mode__to_no),
		TEST_BA(delta_human_mode__from_no),

		TEST_BA(delta_human_adaptive_sync__on),
		TEST_BA(delta_human_adaptive_sync__off),

		TEST_BA(delta_human_reapply__),

		TEST_BA(delta_human__all),
		TEST_BA(delta_human__enabled),
		TEST_BA(delta_human__disabled),
	};

	return RUN(tests);
}

