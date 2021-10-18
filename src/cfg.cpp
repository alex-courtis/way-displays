#include <libgen.h>
#include <linux/limits.h>
#include <string.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

extern "C" {
#include "cfg.h"
#include "log.h"
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

	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s%s/way-displays/%s", prefix, suffix, CFG_FILE_NAME);
	if (access(path, R_OK) != 0) {
		return false;
	}

	char *file_path = realpath(path, NULL);
	if (!file_path) {
		return false;
	}
	if (access(file_path, R_OK) != 0) {
		free(file_path);
		return false;
	}

	cfg->file_path = file_path;

	strcpy(path, file_path);
	cfg->dir_path = strdup(dirname(path));

	strcpy(path, file_path);
	cfg->file_name = strdup(basename(path));

	return true;
}

bool parse(struct Cfg *cfg) {
	if (!cfg || !cfg->file_path)
		return false;

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

		if (config["LOG_THRESHOLD"]) {
			const auto &level = config["LOG_THRESHOLD"].as<string>();
			if (level == "DEBUG") {
				log_threshold = LOG_LEVEL_DEBUG;
			} else if (level == "INFO") {
				log_threshold = LOG_LEVEL_INFO;
			} else if (level == "WARNING") {
				log_threshold = LOG_LEVEL_WARNING;
			} else if (level == "ERROR") {
				log_threshold = LOG_LEVEL_ERROR;
			} else {
				log_threshold = LOG_LEVEL_INFO;
				log_warn("\nIgnoring invalid LOG_THRESHOLD: '%s', using default 'INFO'\n", level.c_str());
			}
		}

		if (config["SCALE"]) {
			const auto &display_scales = config["SCALE"];
			for (const auto &display_scale : display_scales) {
				if (display_scale["NAME_DESC"] && display_scale["SCALE"]) {
					struct UserScale *user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));
					user_scale->name_desc = strdup(display_scale["NAME_DESC"].as<string>().c_str());
					user_scale->scale = display_scale["SCALE"].as<float>();
					if (user_scale->scale <= 0) {
						log_warn("\nIgnoring invalid scale for %s: %.2f\n", user_scale->name_desc, user_scale->scale);
						free(user_scale);
					} else {
						slist_append(&cfg->user_scales, user_scale);
					}
				}
			}
		}

	} catch (const exception &e) {
		log_error("\ncannot read '%s': %s\n", cfg->file_path, e.what());
		return false;
	}
	return true;
}

void print_cfg(struct Cfg *cfg) {
	if (!cfg)
		return;

	struct UserScale *user_scale;
	struct SList *i;

	if (cfg->file_path) {
		log_info("\nRead configuration file: %s\n", cfg->file_path);
	} else {
		log_info("\nNo configuration file found.\n");
	}

	log_info("  Auto scale: %s\n", cfg->auto_scale ? "ON" : "OFF");

	log_info("  Laptop display prefix: '%s'\n", cfg->laptop_display_prefix);

	if (cfg->order_name_desc) {
		log_info("  Order:\n");
		for (i = cfg->order_name_desc; i; i = i->nex) {
			log_info("    %s\n", (char*)i->val);
		}
	}

	if (cfg->user_scales) {
		log_info("  Scale:\n");
		for (i = cfg->user_scales; i; i = i->nex) {
			user_scale = (struct UserScale*)i->val;
			log_info("    %s: %.2f\n", user_scale->name_desc, user_scale->scale);
		}
	}
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

	if (parse(cfg_new)) {
		print_cfg(cfg_new);
		free_cfg(cfg);
		return cfg_new;
	} else {
		free_cfg(cfg_new);
		return cfg;
	}
}

