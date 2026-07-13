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
#include "info/print.h"
#include "log.h"
#include "pslist.h"
#include "yaml/marshal-types.h"
#include "yaml/marshal.h"
#include "yaml/unmarshal-types.h"
#include "yaml/unmarshal.h"

// TODO maybe g_cfg_file_known_paths
// one-shot singleton set via g_cfg_file_paths_init
struct Pslist *g_cfg_file_paths = NULL;

struct CfgFile *g_cfg_file = NULL;

static void set_paths(struct CfgFile *cfg_file, char *resolved_from, const char *file_path) {
	static char path[PATH_MAX];

	cfg_file->resolved_path = resolved_from;

	cfg_file->file_path = strdup(file_path);

	// dirname modifies path
	strncpy(path, cfg_file->file_path, PATH_MAX - 1);
	free(cfg_file->dir_path);
	cfg_file->dir_path = strdup(dirname(path));

	// basename modifies path
	strncpy(path, cfg_file->file_path, PATH_MAX - 1);
	free(cfg_file->file_name);
	cfg_file->file_name = strdup(basename(path));
}

static bool g_cfg_file_resolve(void) {
	if (!g_cfg_file)
		return false;

	g_cfg_file_init();

	for (struct Pslist *i = g_cfg_file_paths; i; i = i->nex) {
		char *path = fs_canonical_path(i->val);
		if (path) {
			set_paths(g_cfg_file, i->val, path);
			free(path);
			return true;
		}
	}

	return false;
}

void g_cfg_file_init(void) {
	g_cfg_file_destroy();
	g_cfg_file = calloc(1, sizeof(struct CfgFile));
}

void g_cfg_file_destroy(void) {
	if (g_cfg_file) {
		free(g_cfg_file->dir_path);
		free(g_cfg_file->file_path);
		free(g_cfg_file->file_name);
		free(g_cfg_file);
	}
	g_cfg_file = NULL;
}

void g_cfg_file_paths_init(const char *user_path) {
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

void g_cfg_file_paths_destroy(void) {
	pslist_free_vals(&g_cfg_file_paths, NULL);
}

static bool g_cfg_file_write_content(const char * const yaml) {
	return
		fs_write_file(g_cfg_file->file_path, COMMENT_YAML_SCHEMA, "w") &&
		fs_write_file(g_cfg_file->file_path, yaml, "a");
}

void g_cfg_file_write(void) {
	char *yaml = NULL;
	const char *resolved_from = g_cfg_file->resolved_path;
	bool written = false;

	g_cfg_file->modified = false;

	if (!(yaml = yaml_marshal(g_cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg"))) {
		goto end;
	}

	if (g_cfg_file->file_path && (written = g_cfg_file_write_content(yaml))) {
		g_cfg_file->modified = true;
		goto end;
	}

	if (!written) {

		// kill that cfg file
		g_cfg_file_init();
		fd_wd_cfg_dir_destroy();

		// write preferred alternatives
		for (struct Pslist *i = g_cfg_file_paths; i; i = i->nex) {

			// skip previously resolved
			if (resolved_from == i->val) {
				continue;
			}

			set_paths(g_cfg_file, i->val, i->val);

			// attempt to write
			if (fs_mkdir_p(g_cfg_file->dir_path, 0755) && (written = g_cfg_file_write_content(yaml))) {

				// watch the new
				fd_wd_cfg_dir_create();
				goto end;
			}

			g_cfg_file_init();
		}
	}

end:
	free(yaml);

	if (written) {
		log_info(NULL);
		log_info("Wrote configuration file: %s", g_cfg_file->file_path);
	}
}

void g_cfg_file_read(void) {
	struct Cfg *cfg_resolved = cfg_init();

	bool resolved = g_cfg_file_resolve();

	if (resolved) {
		log_info(NULL);
		log_info("Found configuration file: %s", g_cfg_file->file_path);

		g_cfg = yaml_unmarshal_file(g_cfg_file->file_path, yaml_root_to_cfg);

		if (!g_cfg) {
			log_info(NULL);
			log_info("Using default configuration:");
			g_cfg = cfg_init();
		}
	} else {
		log_info(NULL);
		log_info("No configuration file found, using defaults:");
		g_cfg = cfg_init();
	}

	cfg_apply_defaults(g_cfg);

	cfg_validate_fix(g_cfg);
	log_info(NULL);
	log_info("Active configuration:");
	print_cfg(INFO, g_cfg, false);
	cfg_validate_warn(g_cfg);

	cfg_free(cfg_resolved);
}

void g_cfg_file_reload(void) {
	if (!g_cfg || !g_cfg_file)
		return;

	char *path = g_cfg_file->file_path;
	if (!path)
		return;

	log_info(NULL);
	log_info("Reloading configuration file: %s", path);

	struct Cfg *cfg_loaded = yaml_unmarshal_file(path, yaml_root_to_cfg);

	if (cfg_loaded) {
		cfg_apply_defaults(cfg_loaded);

		cfg_free(g_cfg);
		g_cfg = cfg_loaded;

		log_set_threshold(g_cfg->log_threshold, false);
		cfg_validate_fix(g_cfg);
		log_info(NULL);
		log_info("New configuration:");
		print_cfg(INFO, g_cfg, false);
		cfg_validate_warn(g_cfg);

	} else {
		log_info(NULL);
		log_info("Configuration unchanged:");
		print_cfg(INFO, g_cfg, false);
	}
}
