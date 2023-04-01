#include "tst.h" // IWYU pragma: keep

#include <cmocka.h>

#include "log.h"

#include "head.h"
#include "info.h"
#include "head.h"

void __wrap_print_head(enum LogThreshold t, enum InfoEvent event, struct Head *head) {
	check_expected(t);
	check_expected(event);
	check_expected(head);
}

void __wrap_print_mode(enum LogThreshold t, struct Mode *mode) {
	check_expected(t);
	check_expected(mode);
}

