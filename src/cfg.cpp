#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

extern "C" {
#include "cfg.h"
#include "list.h"
#include "types.h"
}

namespace { // }
using std::exception;
using std::invalid_argument;
using std::string;
using std::stringstream;
} // namespace

#define DEFAULT_LAPTOP_OUTPUT_PREFIX "eDP"

void print_cfg(struct Cfg *cfg) {
	struct SList *i;

	if (cfg->file_path) {
		printf("\nUsing configuration file: %s\n", cfg->file_path);
	} else {
		printf("\nConfiguration file not found.\n");
	}

	printf("  Laptop display prefix:\n");
	if (cfg->laptop_display_prefix) {
		printf("    '%s'", cfg->laptop_display_prefix);
		if (strcmp(cfg->laptop_display_prefix, DEFAULT_LAPTOP_OUTPUT_PREFIX) == 0) {
			printf("  (default)\n");
		} else {
			printf("\n");
		}
	}

	printf("  Display order name / description:\n");
	if (cfg->order_name_desc) {
		for (i = cfg->order_name_desc; i; i = i->nex) {
			printf("    '%s'\n", (char*)i->val);
		}
	} else {
		printf("    (none specified)\n");
	}
}

struct Cfg *read_cfg(const char *path) {
	YAML::Node config;

	struct Cfg *cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));
	cfg->laptop_display_prefix = strdup(DEFAULT_LAPTOP_OUTPUT_PREFIX);

	if (access(path, R_OK) != 0) {
		cfg->file_path = NULL;
	} else {
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

		} catch (const exception &e) {
			fprintf(stderr, "ERROR: cannot read '%s': %s, exiting\n", path, e.what());
			exit(EX_DATAERR);
		}
	}

	return cfg;
}

