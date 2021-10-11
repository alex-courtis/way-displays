#include <linux/limits.h>
#include <string.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

extern "C" {
#include "cfg.h"
}

namespace { // }
using std::exception;
using std::invalid_argument;
using std::string;
using std::stringstream;
} // namespace

#define CFG_FILE_NAME "cfg.yaml"
#define DEFAULT_LAPTOP_OUTPUT_PREFIX "eDP"

struct Cfg *default_cfg() {
	struct Cfg *cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	cfg->dirty = true;

	cfg->laptop_display_prefix = strdup(DEFAULT_LAPTOP_OUTPUT_PREFIX);
	cfg->auto_scale = true;

	return cfg;
}

bool resolve(struct Cfg *cfg, const char *prefix, const char *suffix) {
	if (!cfg)
		return false;

	char path_dir[PATH_MAX];
	char path_file[PATH_MAX];

	snprintf(path_dir, PATH_MAX,  "%s%s/way-displays", prefix, suffix);
	snprintf(path_file, PATH_MAX, "%s%s/way-displays/%s", prefix, suffix, CFG_FILE_NAME);

	if (access(path_file, R_OK) == 0) {
		cfg->dir_path = strdup(path_dir);
		cfg->file_path = strdup(path_file);
		cfg->file_name = strdup(CFG_FILE_NAME);
		return true;
	} else {
		return false;
	}
}

void parse(struct Cfg *cfg) {
	if (!cfg || !cfg->file_path)
		return;

	try {
		YAML::Node config = YAML::LoadFile(cfg->file_path);

		if (config["LAPTOP_DISPLAY_PREFIX"]) {
			free(cfg->laptop_display_prefix);
			cfg->laptop_display_prefix = strdup(config["LAPTOP_DISPLAY_PREFIX"].as<string>().c_str());
		}

		if (config["ORDER"]) {
			const auto &orders = config["ORDER"];
			for (const auto &order : orders) {
				slist_append(&cfg->order_name_desc, strdup(order.as<string>().c_str()));
			}
		}

		if (config["AUTO_SCALE"]) {
			const auto &orders = config["AUTO_SCALE"];
			cfg->auto_scale = orders.as<bool>();
		}

		if (config["SCALE"]) {
			const auto &display_scales = config["SCALE"];
			for (const auto &display_scale : display_scales) {
				if (display_scale["NAME_DESC"] && display_scale["SCALE"]) {
					struct UserScale *user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));
					user_scale->name_desc = strdup(display_scale["NAME_DESC"].as<string>().c_str());
					user_scale->scale = display_scale["SCALE"].as<float>();
					if (user_scale->scale <= 0) {
						printf("\nIgnoring invalid scale for %s: %.2f\n", user_scale->name_desc, user_scale->scale);
						free(user_scale);
					} else {
						slist_append(&cfg->user_scales, user_scale);
					}
				}
			}
		}

	} catch (const exception &e) {
		fprintf(stderr, "\nERROR: cannot read '%s': %s, exiting\n", cfg->file_path, e.what());
		exit(EXIT_FAILURE);
	}
}

void print_cfg(struct Cfg *cfg) {
	struct UserScale *user_scale;
	struct SList *i;

	if (cfg->file_path) {
		printf("\nConfiguration file: %s\n", cfg->file_path);
	} else {
		printf("\nConfiguration file not found.\n");
	}

	printf("  Auto scale: %s\n", cfg->auto_scale ? "ON" : "OFF");

	printf("  Laptop display prefix: '%s'\n", cfg->laptop_display_prefix);

	if (cfg->order_name_desc) {
		printf("  Order:\n");
		for (i = cfg->order_name_desc; i; i = i->nex) {
			printf("    %s\n", (char*)i->val);
		}
	}

	if (cfg->user_scales) {
		printf("  Scale:\n");
		for (i = cfg->user_scales; i; i = i->nex) {
			user_scale = (struct UserScale*)i->val;
			printf("    %s: %.2f\n", user_scale->name_desc, user_scale->scale);
		}
	}

	fflush(stdout);
}

struct Cfg *load_cfg() {
	bool found = false;

	struct Cfg *cfg = default_cfg();

	if (getenv("XDG_CONFIG_HOME"))
		found = resolve(cfg, getenv("XDG_CONFIG_HOME"), "");
	if (!found && getenv("HOME"))
		found = resolve(cfg, getenv("HOME"), "/.config");
	if (!found)
		found = resolve(cfg, "/usr/local/etc", "");
	if (!found)
		found = resolve(cfg, "/etc", "");

	if (found) {
		parse(cfg);
	}

	return cfg;
}

struct Cfg *reload_cfg(struct Cfg *cfg) {
	if (!cfg || !cfg->file_path)
		return cfg;

	struct Cfg *cfg_new = default_cfg();
	cfg_new->dir_path = strdup(cfg->dir_path);
	cfg_new->file_path = strdup(cfg->file_path);
	cfg_new->file_name = strdup(cfg->file_name);

	parse(cfg_new);

	print_cfg(cfg_new);

	free_cfg(cfg);

	return cfg_new;
}

