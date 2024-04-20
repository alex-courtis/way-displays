#include "tst.h" // IWYU pragma: keep

#include <cmocka.h>

void __wrap_spawn_sh_cmd(const char * const command) {
	check_expected(command);
}

void __wrap_wd_exit(int __status) {
	check_expected(__status);
}

void __wrap_wd_exit_message(int __status) {
	check_expected(__status);
}
