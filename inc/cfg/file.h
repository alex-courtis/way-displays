#ifndef CFG_FILE_H
#define CFG_FILE_H

#include <limits.h>
#include <stdbool.h>

struct CfgFile {
	char dir_path[PATH_MAX];
	char file_path[PATH_MAX];
	char file_name[PATH_MAX];
	char *resolved_from;

	bool modified; // pfd_cfg_dir
};

extern struct CfgFile g_cfg_file;

// init g_cfg_file and read a file into g_cfg, preferring user_path, default when no file
void g_cfg_file_init_read(const char *user_path);

// release g_cfg_file
void g_cfg_file_destroy(void);

// write g_cfg to the g_cfg_file
void g_cfg_file_write(void);

// reload g_cfg from g_cfg_file
void g_cfg_file_reload(void);

#endif // CFG_FILE_H
