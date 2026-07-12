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


// TODO remove or move
extern struct Pslist *g_cfg_file_paths;
extern struct CfgFile *g_cfg_file;
void set_paths(struct CfgFile *cfg_file, char *resolved_from, const char *file_path);

// TODO name these all consistently, maybe g_cfg_file_init

/*
 * lifecycle - g_cfg
 */

// instantiate g_cfg_file, destroying if present
void cfg_file_init_global(void);

// free and set g_cfg_file to NULL
void cfg_file_destroy_global(void);

/*
 * lifecycle - g_cfg_file_paths
 */

// populate g_cfg_file_paths, one shot
void cfg_file_paths_init(const char *user_path);

// free all g_cfg_file_paths
void cfg_file_paths_destroy(void);

/*
 * read/write - g_cfg, g_cfg_file
 */

// write g_cfg to the g_cfg_file
void cfg_file_write(void);

// find and read a config file into g_cfg, setting g_cfg_file
void cfg_file_read(void);

// reload g_cfg from g_cfg_file
void cfg_file_reload(void);

/*
 * update
 */

// if a file is found in g_cfg_file_paths, return true and set them
bool cfg_file_resolve(void);

#endif // CFG_FILE_H
