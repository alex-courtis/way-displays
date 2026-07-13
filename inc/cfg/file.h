#ifndef CFG_FILE_H
#define CFG_FILE_H

#include <limits.h>
#include <stdbool.h>

struct CfgFile {
	char dir_path[PATH_MAX];  // inotify pfd_cfg_dir
	char file_path[PATH_MAX];
	char file_name[PATH_MAX]; // name to check on inotify pfd_cfg_dir
	char *file_path_resolved; // --config or expected paths

	bool written;             // set on write to prevent fs watch reloading it
};

extern struct CfgFile g_cfg_file;

// init read a file into g_cfg and g_cfg_file, preferring user_path, when no file default g_cfg and empty g_cfg_file
void g_cfg_file_init_read(const char *user_path);

// release g_cfg_file and other resources
void g_cfg_file_destroy(void);

// write g_cfg to the g_cfg_file
void g_cfg_file_write(void);

// reload g_cfg from g_cfg_file
void g_cfg_file_reload(void);

#endif // CFG_FILE_H
