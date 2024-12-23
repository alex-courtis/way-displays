#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "head.h"
#include "log.h"
#include "mode.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"

#include "listeners.h"

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	return 0;
}

int after_each(void **state) {
	assert_logs_empty();

	return 0;
}

void preferred__first(void **state) {
	struct Head head = { .name = "NAM", };
	struct Mode mode_existing = { .width = 3840, .height = 2160, .preferred = false, .refresh_mhz = 60000, .head = &head, };
	struct Mode mode_preferred = { .width = 2560, .height = 1440, .preferred = false, .refresh_mhz = 30000, .head = &head, };

	slist_append(&head.modes, &mode_existing);
	slist_append(&head.modes, &mode_preferred);

	zwlr_output_mode_listener()->preferred(&mode_preferred, NULL);

	assert_false(mode_existing.preferred);
	assert_true(mode_preferred.preferred);

	slist_free(&head.modes);
}

void preferred__subsequent(void **state) {
	struct Head head = { .name = "NAM", };
	struct Mode mode_existing = { .width = 3840, .height = 2160, .preferred = true, .refresh_mhz = 60000, .head = &head, };
	struct Mode mode_subsequent = { .width = 2560, .height = 1440, .preferred = false, .refresh_mhz = 30000, .head = &head, };

	slist_append(&head.modes, &mode_existing);
	slist_append(&head.modes, &mode_subsequent);

	zwlr_output_mode_listener()->preferred(&mode_subsequent, NULL);

	assert_log(INFO, "\nNAM: multiple preferred modes advertised: using initial 3840x2160@60Hz (60,000mHz) (preferred), ignoring 2560x1440@30Hz (30,000mHz)\n");

	assert_true(mode_existing.preferred);
	assert_false(mode_subsequent.preferred);

	slist_free(&head.modes);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(preferred__first),
		TEST(preferred__subsequent)
	};

	return RUN(tests);
}

