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

#define DEFAULT_LAPTOP_OUTPUT_PREFIX "eDP"

bool access_cfg(char *path, const char *prefix, const char *suffix) {
	snprintf(path, PATH_MAX, "%s%s/way-displays/cfg.yaml", prefix, suffix);
	return access(path, R_OK) == 0;
}

void print_cfg(struct Cfg *cfg) {
	struct UserScale *user_scale;
	struct SList *i;

	if (cfg->file_path) {
		printf("\nConfiguration file: %s\n", cfg->file_path);
	} else {
		printf("\nConfiguration file not found.\n");
	}

	printf("  Auto scale:\n");
	if (cfg->auto_scale) {
		printf("    ON\n");
	} else {
		printf("    OFF\n");
	}

	printf("  Laptop display prefix:\n");
	if (cfg->laptop_display_prefix) {
		printf("    %s\n", cfg->laptop_display_prefix);
	}

	if (cfg->order_name_desc) {
		printf("  Display order:\n");
		for (i = cfg->order_name_desc; i; i = i->nex) {
			printf("    %s\n", (char*)i->val);
		}
	}

	if (cfg->user_scales) {
		printf("  Display scales:\n");
		for (i = cfg->user_scales; i; i = i->nex) {
			user_scale = (struct UserScale*)i->val;
			printf("    %s: %.2f\n", user_scale->name_desc, user_scale->scale);
		}
	}
}

struct Cfg *read_cfg() {
	YAML::Node config;
	char path[PATH_MAX];
	bool found = false;

	struct Cfg *cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));
	cfg->laptop_display_prefix = strdup(DEFAULT_LAPTOP_OUTPUT_PREFIX);
	cfg->auto_scale = true;

	if (getenv("XDG_CONFIG_HOME"))
		found = access_cfg(path, getenv("XDG_CONFIG_HOME"), "");
	if (!found && getenv("HOME"))
		found = access_cfg(path, getenv("HOME"), "/.config");
	if (!found)
		found = access_cfg(path, "/etc", "");

	if (found) {
		cfg->file_path = strdup(path);
		try {
			config = YAML::LoadFile(path);

			if (config["LAPTOP_DISPLAY_PREFIX"]) {
				free(cfg->laptop_display_prefix);
				cfg->laptop_display_prefix = strdup(config["LAPTOP_DISPLAY_PREFIX"].as<string>().c_str());
			}

			if (config["ORDER_NAME_DESC"]) {
				const auto &orders = config["ORDER_NAME_DESC"];
				for (const auto &order : orders) {
					slist_append(&cfg->order_name_desc, strdup(order.as<string>().c_str()));
				}
			}

			if (config["AUTO_SCALE"]) {
				const auto &orders = config["AUTO_SCALE"];
				cfg->auto_scale = orders.as<bool>();
			}

			if (config["DISPLAY_SCALE"]) {
				const auto &display_scales = config["DISPLAY_SCALE"];
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
			fprintf(stderr, "\nERROR: cannot read '%s': %s, exiting\n", path, e.what());
			exit(EXIT_FAILURE);
		}
	}

	return cfg;
}

