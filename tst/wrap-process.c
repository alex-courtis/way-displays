#include "tst.h" // IWYU pragma: keep

#include <cmocka.h>

#include "stable.h"

void __wrap_spawn_sh_cmd(const char * const command, const struct STable * const env) {
	check_expected_ptr(command);
	check_expected_ptr(env);
}

void __wrap_wd_exit(const int __status) {
	check_expected_int(__status);
}

void __wrap_wd_exit_message(const int __status) {
	check_expected_int(__status);
}
