#ifndef CFG_FILE_H
#define CFG_FILE_H

#include <stdbool.h>

struct CfgFile {
	char *dir_path;
	char *file_path;
	char *file_name;
	char *resolved_path;

	bool modified; // pfd_cfg_dir
};

extern struct CfgFile *g_cfg_file;

/*
 * lifecycle - file
 */

// instantiate g_cfg_file, destroying if present
void g_cfg_file_init(void);

// free and set g_cfg_file to NULL
void g_cfg_file_destroy(void);

/*
 * lifecycle - paths
 */

// TODO consolidate with g_cfg_file_init

// populate g_cfg_file_paths with known paths and user_path if exists
void g_cfg_file_paths_init(const char *user_path);

// free all g_cfg_file_paths
void g_cfg_file_paths_destroy(void);

/*
 * read/write - g_cfg, g_cfg_file
 */

// write g_cfg to the g_cfg_file
void g_cfg_file_write(void);

// find and read a config file into g_cfg, setting g_cfg_file
void g_cfg_file_read(void);

// reload g_cfg from g_cfg_file
void g_cfg_file_reload(void);

#endif // CFG_FILE_H
