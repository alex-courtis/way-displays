#ifndef CFG_FILE_H
#define CFG_FILE_H

#include <stdbool.h>

struct CfgFile {
	char *dir_path;
	char *file_path;
	char *file_name;
	char *resolved_from;

	bool modified;                           // pfd_cfg_dir
};

/*
 * lifecycle - CfgFile
 */

void cfg_file_free(struct CfgFile *cfg_file);

/*
 * lifecycle - g_cfg_file_paths
 */

// populate g_cfg_file_paths, one shot
void cfg_file_paths_init(const char *user_path);

// free all g_cfg_file_paths
void cfg_file_paths_destroy(void);

/*
 * execute
 */

// write g_cfg to the appropriate file path
void cfg_file_write(void);

/*
 * update
 */

// if a file is found in g_cfg_file_paths, return true and set them
bool cfg_file_resolve(struct CfgFile *cfg_file);

// duplicate paths
void cfg_file_merge(struct CfgFile *to, const struct CfgFile *from);

#endif // CFG_FILE_H
