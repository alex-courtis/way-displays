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

void __wrap_report_outcome(enum LogThreshold t) {
	check_expected(t);
}

void __wrap_report_outcome_adaptive_sync_fail(void) {
	function_called();
}

