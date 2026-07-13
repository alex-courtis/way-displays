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

struct Pslist *g_cfg_file_paths = NULL;

struct CfgFile g_cfg_file = { 0 };

static void cfg_file_populate(char *resolved_from, const char *paths) {
	static char tmp[PATH_MAX];

	g_cfg_file.resolved_from = resolved_from;

	strncpy(g_cfg_file.file_path, paths, PATH_MAX - 1);

	// dirname modifies path
	strncpy(tmp, g_cfg_file.file_path, PATH_MAX - 1);
	strncpy(g_cfg_file.dir_path, dirname(tmp), PATH_MAX - 1);

	// basename modifies path
	strncpy(tmp, g_cfg_file.file_path, PATH_MAX - 1);
	strncpy(g_cfg_file.file_name, basename(tmp), PATH_MAX - 1);
}

static bool g_cfg_file_resolve(void) {
	memset(&g_cfg_file, 0, sizeof(struct CfgFile));

	for (struct Pslist *i = g_cfg_file_paths; i; i = i->nex) {
		if (!i->val)
			continue;
		char *path = fs_canonical_path(i->val);
		if (path) {
			cfg_file_populate(i->val, path);
			free(path);
			return true;
		}
	}

	return false;
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

static bool g_cfg_file_write_content(const char * const yaml) {
	return
		fs_file_write(g_cfg_file.file_path, COMMENT_YAML_SCHEMA, "w") &&
		fs_file_write(g_cfg_file.file_path, yaml, "a");
}

void g_cfg_file_write(void) {
	char *yaml = NULL;
	bool written = false;

	const char *resolved_from_prev = g_cfg_file.resolved_from;

	g_cfg_file.modified = false;

	if (!(yaml = yaml_marshal(g_cfg, (fn_yaml_root_from_type)yaml_root_from_cfg, "cfg"))) {
		goto end;
	}

	if (strlen(g_cfg_file.file_path) > 0 && (written = g_cfg_file_write_content(yaml))) {
		g_cfg_file.modified = true;
		goto end;
	}

	// kill that cfg file
	memset(&g_cfg_file, 0, sizeof(struct CfgFile));

	fd_wd_cfg_dir_destroy();

	// write preferred alternatives
	for (const struct Pslist *i = g_cfg_file_paths; i; i = i->nex) {

		// skip previously resolved
		if (resolved_from_prev == i->val) {
			continue;
		}

		// optimistically populate
		cfg_file_populate(i->val, i->val);

		// attempt to write
		if (fs_mkdir_p(g_cfg_file.dir_path, 0755) && (written = g_cfg_file_write_content(yaml))) {

			// watch the new
			fd_wd_cfg_dir_create();
			goto end;
		}

		// clear on failure
		memset(&g_cfg_file, 0, sizeof(struct CfgFile));
	}

end:
	free(yaml);

	if (written) {
		log_info(NULL);
		log_info("Wrote configuration file: %s", g_cfg_file.file_path);
	}
}

void g_cfg_file_init_read(const char *user_path) {
	struct Cfg *cfg_resolved = cfg_init();

	// one shot
	if (!g_cfg_file_paths) {
		g_cfg_file_paths_init(user_path);
	}

	bool resolved = g_cfg_file_resolve();

	if (resolved) {
		log_info(NULL);
		log_info("Found configuration file: %s", g_cfg_file.file_path);

		g_cfg = yaml_unmarshal_file(g_cfg_file.file_path, yaml_root_to_cfg);

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
	if (strlen(g_cfg_file.file_path) == 0)
		return;

	log_info(NULL);
	log_info("Reloading configuration file: %s", g_cfg_file.file_path);

	struct Cfg *cfg_loaded = yaml_unmarshal_file(g_cfg_file.file_path, yaml_root_to_cfg);

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

void g_cfg_file_destroy(void) {
	pslist_free_vals(&g_cfg_file_paths, NULL);
}

