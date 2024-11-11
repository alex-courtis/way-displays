#include "tst.h" // IWYU pragma: keep

#include <cmocka.h>

void __wrap_spawn_sh_cmd(const char * const command, const char * const message) {
	check_expected(command);
	check_expected(message);
}

void __wrap_wd_exit(int __status) {
	check_expected(__status);
}

void __wrap_wd_exit_message(int __status) {
	check_expected(__status);
}
