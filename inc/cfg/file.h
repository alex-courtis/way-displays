#ifndef CFG_FILE_H
#define CFG_FILE_H

#include <stdbool.h>

#include "cfg.h"

/*
 * lifecycle - Cfg
 */

void cfg_paths_free(struct Cfg *cfg);

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

// if a file is found in g_cfg_file_paths, return true and set them in to
bool cfg_resolve_file_path(struct Cfg *to);

// duplicate paths
void cfg_copy_file_path(struct Cfg *to, const struct Cfg *from);

#endif // CFG_FILE_H
