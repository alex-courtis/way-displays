#include "tst.h" // IWYU pragma: keep
#include "expects.h"

#include <cmocka.h>
#include <stdio.h>
#include <stdbool.h>

#include "log.h"

#include "info.h"

void __wrap_print_head(enum LogThreshold t, enum InfoEvent event, struct Head *head) {
	check_expected(t);
	check_expected(event);
	check_expected(head);
}

