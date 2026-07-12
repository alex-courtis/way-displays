#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cfg/file.h"

#include "cfg.h"
#include "fds.h"
#include "fs.h"
#include "log.h"
#include "pslist.h"
#include "yaml/marshal-types.h"
#include "yaml/marshal.h"

// one-shot singleton set via cfg_file_paths_init
struct Pslist *g_cfg_file_paths = NULL;

static void set_paths(struct Cfg *cfg, char *resolved_from, const char *file_path) {
	static char path[PATH_MAX];

	cfg->file.resolved_from = resolved_from;

	cfg->file.file_path = strdup(file_path);

	// dirname modifies path
	strncpy(path, cfg->file.file_path, PATH_MAX - 1);
	free(cfg->file.dir_path);
	cfg->file.dir_path = strdup(dirname(path));

	// basename modifies path
	strncpy(path, cfg->file.file_path, PATH_MAX - 1);
	free(cfg->file.file_name);
	cfg->file.file_name = strdup(basename(path));
}

// TODO explicit test or move to correct module
static bool cfg_file_write_content(const char * const yaml) {
	return
		file_write(g_cfg->file.file_path, COMMENT_YAML_SCHEMA, "w") &&
		file_write(g_cfg->file.file_path, yaml, "a");
}

void cfg_file_paths_init(const char *user_path) {
	char path[PATH_MAX];

	// maybe user
	if (user_path && access(user_path, R_OK) == 0) {
		pslist_append(&g_cfg_file_paths, strdup(user_path));
	}

	if (getenv("XDG_CONFIG_HOME") != NULL) {
		// maybe XDG_CONFIG_HOME
		snprintf(path, PATH_MAX - 1, "%s/way-displays/cfg.yaml", getenv("XDG_CONFIG_HOME"));
		pslist_append(&g_cfg_file_paths, strdup(path));
	} else if (getenv("HOME") != NULL) {
		// ~/.config
		snprintf(path, PATH_MAX - 1, "%s/.config/way-displays/cfg.yaml", getenv("HOME"));
		pslist_append(&g_cfg_file_paths, strdup(path));
	}

	// etc
	pslist_append(&g_cfg_file_paths, strdup("/usr/local/etc/way-displays/cfg.yaml"));
	pslist_append(&g_cfg_file_paths, strdup(ROOT_ETC"/way-displays/cfg.yaml"));
}

void cfg_file_paths_destroy(void) {
	pslist_free_vals(&g_cfg_file_paths, NULL);
}

void cfg_paths_free(struct Cfg *cfg) {
	free(cfg->file.dir_path);
	cfg->file.dir_path = NULL;

	free(cfg->file.file_path);
	cfg->file.file_path = NULL;

	free(cfg->file.file_name);
	cfg->file.file_name = NULL;

	cfg->file.resolved_from = NULL;
}

// TODO explicit test or move to correct module
void cfg_file_write(void) {
	char *yaml = NULL;
	const char *resolved_from = g_cfg->file.resolved_from;
	bool written = false;

	g_cfg->file.modified = false;

	if (!(yaml = yaml_marshal(g_cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg"))) {
		goto end;
	}

	if (g_cfg->file.file_path && (written = cfg_file_write_content(yaml))) {
		g_cfg->file.modified = true;
		goto end;
	}

	if (!written) {

		// kill that cfg file
		cfg_paths_free(g_cfg);
		fd_wd_cfg_dir_destroy();

		// write preferred alternatives
		for (struct Pslist *i = g_cfg_file_paths; i; i = i->nex) {

			// skip previously resolved
			if (resolved_from == i->val) {
				continue;
			}

			set_paths(g_cfg, i->val, i->val);

			// attempt to write
			if (mkdir_p(g_cfg->file.dir_path, 0755) && (written = cfg_file_write_content(yaml))) {

				// watch the new
				fd_wd_cfg_dir_create();
				goto end;
			}

			cfg_paths_free(g_cfg);
		}
	}

end:
	free(yaml);

	if (written) {
		log_info(NULL);
		log_info("Wrote configuration file: %s", g_cfg->file.file_path);
	}
}

bool cfg_resolve_file_path(struct Cfg *to) {
	if (!to)
		return false;

	cfg_paths_free(to);

	for (struct Pslist *i = g_cfg_file_paths; i; i = i->nex) {
		if (access(i->val, R_OK) == 0) {

			char *file_path = realpath(i->val, NULL);

			if (!file_path) {
				continue;
			}
			if (access(file_path, R_OK) != 0) {
				free(file_path);
				continue;
			}

			set_paths(to, i->val, file_path);

			free(file_path);

			return true;
		}
	}

	return false;
}

void cfg_copy_file_path(struct Cfg *to, const struct Cfg *from) {
	if (!from || !to)
		return;

	free(to->file.dir_path);
	free(to->file.file_path);
	free(to->file.file_name);

	to->file.dir_path = from->file.dir_path ? strdup(from->file.dir_path) : NULL;
	to->file.file_path = from->file.file_path ? strdup(from->file.file_path) : NULL;
	to->file.file_name = from->file.file_name ? strdup(from->file.file_name) : NULL;
}
