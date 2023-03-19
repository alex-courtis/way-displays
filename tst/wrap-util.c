#include "tst.h" // IWYU pragma: keep

#include <cmocka.h>

extern void __wrap_wd_exit(int __status) {
	check_expected(__status);
}
