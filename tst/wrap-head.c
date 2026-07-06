#include <cmocka.h>
#include <wayland-util.h>

#include "head.h"

const struct Mode *__wrap_head_find_mode(struct Head * const head) {
	check_expected_ptr(head);
	return mock_ptr_type_checked(struct Mode*);
}

wl_fixed_t __wrap_head_auto_scale(struct Head *head) {
	check_expected_ptr(head);
	return mock_type(wl_fixed_t);
}
