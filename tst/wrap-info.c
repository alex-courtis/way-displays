#include "tst.h" // IWYU pragma: keep

#include <cmocka.h>

#include "log.h"

#include "head.h"
#include "info.h"
#include "mode.h"

void __wrap_print_head(enum LogThreshold t, enum InfoEvent event, struct Head *head) {
	check_expected(t);
	check_expected(event);
	check_expected(head);
}

void __wrap_print_mode(enum LogThreshold t, struct Mode *mode) {
	check_expected(t);
	check_expected(mode);
}

void __wrap_call_back(enum LogThreshold t, const char * const msg1, const char * const msg2) {
	check_expected(t);
	check_expected(msg1);
	check_expected(msg2);
}

void __wrap_call_back_adaptive_sync_fail(enum LogThreshold t, const struct Head * const head) {
	check_expected(t);
	check_expected(head);
}

