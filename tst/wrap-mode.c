#include <cmocka.h>

#include "pset.h"

#include "mode.h"

double __wrap_mode_dpi(const struct Mode* const mode) {
	check_expected_ptr(mode);
	return mock_type(double);
}

const struct Mode *__wrap_mode_best_satisfying(const struct Mode * const mode_target, const struct PSet* const modes) {
	check_expected_ptr(mode_target);
	check_expected_ptr(modes);
	return mock_ptr_type_checked(struct Mode*);
}

const struct Mode *__wrap_mode_max_refresh(const struct Mode* const mode_target, const struct PSet* modes) {
	check_expected_ptr(mode_target);
	check_expected_ptr(modes);
	return mock_ptr_type_checked(struct Mode*);
}

