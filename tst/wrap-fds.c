#include <cmocka.h>
#include <stdbool.h>
#include <sys/types.h>

bool __wrap_file_write(const char *path, const char *contents, const char *mode) {
	check_expected_ptr(path);
	check_expected_ptr(contents);
	check_expected_ptr(mode);
	return mock_type(bool);
}

bool __wrap_mkdir_p(char *path, mode_t mode) {
	check_expected_ptr(path);
	check_expected_int(mode);
	return mock_type(bool);
}

void __wrap_fd_wd_cfg_dir_create(void) {
	function_called();
}

void __wrap_fd_wd_cfg_dir_destroy(void) {
	function_called();
}
