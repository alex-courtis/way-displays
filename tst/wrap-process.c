#include "tst.h" // IWYU pragma: keep

#include <cmocka.h>

#include "stable.h"

void __wrap_spawn_sh_cmd(const char * const command, const struct STable * const env) {
	check_expected(command);
	check_expected(env);
}

void __wrap_wd_exit(const int __status) {
	check_expected(__status);
}

void __wrap_wd_exit_message(const int __status) {
	check_expected(__status);
}
