#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "laptop.h"

#define LAPTOP_LID_ROOT_PATH "/proc/acpi/button/lid"
#define LAPTOP_OUTPUT_PREFIX "eDP"

bool closed_laptop_display(const char *name) {
	return
		strncasecmp(LAPTOP_OUTPUT_PREFIX, name, strlen(LAPTOP_OUTPUT_PREFIX)) == 0 &&
		laptop_lid_closed(LAPTOP_LID_ROOT_PATH);
}

bool laptop_lid_closed(const char *root_path) {
	static char lidFileName[PATH_MAX];
	static char line[512];
	static bool closed;

	// find the lid state directory
	DIR *dir = opendir(root_path);
	if (dir) {
		struct dirent *dirent;
		while ((dirent = readdir(dir)) != NULL) {
			if (dirent->d_type == DT_DIR && strcmp(dirent->d_name, ".") != 0 && strcmp(dirent->d_name, "..") != 0) {

				// read the lid state file
				snprintf(lidFileName, PATH_MAX, "%s/%s/%s", root_path, dirent->d_name, "state");
				FILE *lidFile = fopen(lidFileName, "r");
				if (lidFile) {
					if (fgets(line, 512, lidFile)) {
						closed = strcasestr(line, "closed");
					}
					fclose(lidFile);
				}

				// drivers/acpi/button.c acpi_button_add_fs seems to indicate there will be only one file
				break;
			}
		}
		closedir(dir);
	}
	return closed;
}

