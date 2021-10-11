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

bool resolve(struct Cfg *cfg, const char *prefix, const char *suffix) {
	if (!cfg)
		return false;

	char path_dir[PATH_MAX];
	char path_file[PATH_MAX];

	snprintf(path_dir, PATH_MAX,  "%s%s/way-displays", prefix, suffix);
	snprintf(path_file, PATH_MAX, "%s%s/way-displays/%s", prefix, suffix, CFG_FILE_NAME);

	if (access(path_file, R_OK) == 0) {
		cfg->path_dir = strdup(path_dir);
		cfg->path_file = strdup(path_file);
		return true;
	} else {
		return false;
	}
}

void parse(struct Cfg *cfg) {
	if (!cfg || !cfg->path_file)
		return;

	try {
		YAML::Node config = YAML::LoadFile(cfg->path_file);

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
		fprintf(stderr, "\nERROR: cannot read '%s': %s, exiting\n", cfg->path_file, e.what());
		exit(EXIT_FAILURE);
	}
}

void print_cfg(struct Cfg *cfg) {
	struct UserScale *user_scale;
	struct SList *i;

	if (cfg->path_file) {
		printf("\nConfiguration file: %s\n", cfg->path_file);
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

struct Cfg *read_cfg() {
	bool found = false;

	struct Cfg *cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));
	cfg->laptop_display_prefix = strdup(DEFAULT_LAPTOP_OUTPUT_PREFIX);
	cfg->auto_scale = true;

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

