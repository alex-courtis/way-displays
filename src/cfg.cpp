#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h> // IWYU pragma: keep
#include <yaml-cpp/emitter.h>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/detail/iterator.h>
#include <yaml-cpp/node/impl.h>
#include <yaml-cpp/node/iterator.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <exception>
#include <stdexcept>
#include <string>

#include "cfg.h"

extern "C" {
#include "convert.h"
#include "info.h"
#include "list.h"
#include "log.h"
}

enum Arrange ARRANGE_DEFAULT = ROW;
enum Align ALIGN_DEFAULT = TOP;
enum AutoScale AUTO_SCALE_DEFAULT = ON;

bool slist_test_scale_name(const void *value, const void *data) {
	if (!value || !data) {
		return false;
	}

	struct UserScale *lhs = (struct UserScale*)value;
	struct UserScale *rhs = (struct UserScale*)data;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcasecmp(lhs->name_desc, rhs->name_desc) == 0;
}

bool slist_test_scale_name_val(const void *value, const void *data) {
	if (!value || !data) {
		return false;
	}

	struct UserScale *lhs = (struct UserScale*)value;
	struct UserScale *rhs = (struct UserScale*)data;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcasecmp(lhs->name_desc, rhs->name_desc) == 0 && lhs->scale == rhs->scale;
}

struct Cfg *cfg_clone(struct Cfg *from) {
	if (!from) {
		return NULL;
	}

	struct SList *i;
	struct Cfg *to = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	to->dir_path = from->dir_path ? strdup(from->dir_path) : NULL;
	to->file_path = from->file_path ? strdup(from->file_path) : NULL;
	to->file_name = from->file_name ? strdup(from->file_name) : NULL;

	// ARRANGE
	if (from->arrange) {
		to->arrange = from->arrange;
	}

	// ALIGN
	if (from->align) {
		to->align = from->align;
	}

	// ORDER
	for (i = from->order_name_desc; i; i = i->nex) {
		slist_append(&to->order_name_desc, strdup((char*)i->val));
	}

	// AUTO_SCALE
	if (from->auto_scale) {
		to->auto_scale = from->auto_scale;
	}

	// SCALE
	for (i = from->user_scales; i; i = i->nex) {
		struct UserScale *from_scale = (struct UserScale*)i->val;
		struct UserScale *to_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));
		to_scale->name_desc = strdup(from_scale->name_desc);
		to_scale->scale = from_scale->scale;
		slist_append(&to->user_scales, to_scale);
	}

	// LAPTOP_DISPLAY_PREFIX
	if (from->laptop_display_prefix) {
		to->laptop_display_prefix = strdup(from->laptop_display_prefix);
	}

	// MAX_PREFERRED_REFRESH
	for (i = from->max_preferred_refresh_name_desc; i; i = i->nex) {
		slist_append(&to->max_preferred_refresh_name_desc, strdup((char*)i->val));
	}

	// DISABLED
	for (i = from->disabled_name_desc; i; i = i->nex) {
		slist_append(&to->disabled_name_desc, strdup((char*)i->val));
	}

	// LOG_THRESHOLD
	if (from->log_threshold) {
		to->log_threshold = from->log_threshold;
	}

	return to;
}

bool cfg_equal(struct Cfg *a, struct Cfg* b) {
	if (!a || !b) {
		return false;
	}

	// ARRANGE
	if (a->arrange != b->arrange) {
		return false;
	}

	// ALIGN
	if (a->align != b->align) {
		return false;
	}

	// ORDER
	if (!slist_equal(a->order_name_desc, b->order_name_desc, slist_test_strcasecmp)) {
		return false;
	}

	// AUTO_SCALE
	if (a->auto_scale != b->auto_scale) {
		return false;
	}

	// SCALE
	if (!slist_equal(a->user_scales, b->user_scales, slist_test_scale_name_val)) {
		return false;
	}

	// LAPTOP_DISPLAY_PREFIX
	char *al = a->laptop_display_prefix;
	char *bl = b->laptop_display_prefix;
	if ((al && !bl) || (!al && bl) || (al && bl && strcasecmp(al, bl) != 0)) {
		return false;
	}

	// MAX_PREFERRED_REFRESH
	if (!slist_equal(a->max_preferred_refresh_name_desc, b->max_preferred_refresh_name_desc, slist_test_strcasecmp)) {
		return false;
	}

	// DISABLED
	if (!slist_equal(a->disabled_name_desc, b->disabled_name_desc, slist_test_strcasecmp)) {
		return false;
	}

	// LOG_THRESHOLD
	if (a->log_threshold != b->log_threshold) {
		return false;
	}

	return true;
}

struct Cfg *cfg_default() {
	struct Cfg *cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	cfg->dirty = true;

	cfg->arrange = ARRANGE_DEFAULT;
	cfg->align = ALIGN_DEFAULT;
	cfg->auto_scale = AUTO_SCALE_DEFAULT;

	return cfg;
}

bool cfg_resolve_paths(struct Cfg *cfg, const char *prefix, const char *suffix) {
	if (!cfg)
		return false;

	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s%s/way-displays/cfg.yaml", prefix, suffix);
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

void cfg_parse_node(struct Cfg *cfg, YAML::Node &node) {
	if (!cfg || !node || !node.IsMap()) {
		throw std::runtime_error("missing cfg");
	}

	if (node["LOG_THRESHOLD"]) {
		const std::string &threshold_str = node["LOG_THRESHOLD"].as<std::string>();
		cfg->log_threshold = log_threshold_val(threshold_str.c_str());
		if (!cfg->log_threshold) {
			log_warn("Ignoring invalid LOG_THRESHOLD %s, using default %s", threshold_str.c_str(), log_threshold_name(LOG_THRESHOLD_DEFAULT));
		}
	}

	if (node["LAPTOP_DISPLAY_PREFIX"]) {
		if (cfg->laptop_display_prefix) {
			free(cfg->laptop_display_prefix);
		}
		cfg->laptop_display_prefix = strdup(node["LAPTOP_DISPLAY_PREFIX"].as<std::string>().c_str());
	}

	if (node["ORDER"]) {
		const auto &orders = node["ORDER"];
		for (const auto &order : orders) {
			const std::string &order_str = order.as<std::string>();
			if (!slist_find(&cfg->order_name_desc, slist_test_strcasecmp, order_str.c_str())) {
				slist_append(&cfg->order_name_desc, strdup(order_str.c_str()));
			}
		}
	}

	if (node["ARRANGE"]) {
		const std::string &arrange_str = node["ARRANGE"].as<std::string>();
		enum Arrange arrange = arrange_val_start(arrange_str.c_str());
		if (arrange) {
			cfg->arrange = arrange;
		} else {
			cfg->arrange = ARRANGE_DEFAULT;
			log_warn("Ignoring invalid ARRANGE %s, using default %s", arrange_str.c_str(), arrange_name(cfg->arrange));
		}
	}

	if (node["ALIGN"]) {
		const std::string &align_str = node["ALIGN"].as<std::string>();
		enum Align align = align_val_start(align_str.c_str());
		if (align) {
			cfg->align = align;
		} else {
			cfg->align = ALIGN_DEFAULT;
			log_warn("Ignoring invalid ALIGN %s, using default %s", align_str.c_str(), align_name(cfg->align));
		}
	}

	if (node["AUTO_SCALE"]) {
		const auto &auto_scale = node["AUTO_SCALE"];
		try {
			if (auto_scale.as<bool>()) {
				cfg->auto_scale = ON;
			} else {
				cfg->auto_scale = OFF;
			}
		} catch (YAML::BadConversion &e) {
			cfg->auto_scale = AUTO_SCALE_DEFAULT;
			log_warn("Ignoring invalid AUTO_SCALE %s, using default %s", auto_scale.as<std::string>().c_str(), auto_scale_name(cfg->auto_scale));
		}
	}

	if (node["SCALE"]) {
		const auto &display_scales = node["SCALE"];
		for (const auto &display_scale : display_scales) {
			if (display_scale["NAME_DESC"] && display_scale["SCALE"]) {
				struct UserScale *user_scale = NULL;
				try {
					user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));
					user_scale->name_desc = strdup(display_scale["NAME_DESC"].as<std::string>().c_str());
					try {
						user_scale->scale = display_scale["SCALE"].as<float>();
						if (user_scale->scale <= 0) {
							log_warn("Ignoring invalid SCALE %s %.3f", user_scale->name_desc, user_scale->scale);
							free_user_scale(user_scale);
						} else {
							slist_remove_all_free(&cfg->user_scales, slist_test_scale_name, user_scale, free_user_scale);
							slist_append(&cfg->user_scales, user_scale);
						}
					} catch (YAML::BadConversion &e) {
						log_warn("Ignoring invalid SCALE %s %s", user_scale->name_desc, display_scale["SCALE"].as<std::string>().c_str());
						free_user_scale(user_scale);
					}
				} catch (...) {
					if (user_scale) {
						free_user_scale(user_scale);
					}
					throw;
				}
			}
		}
	}

	if (node["MAX_PREFERRED_REFRESH"]) {
		const auto &name_desc = node["MAX_PREFERRED_REFRESH"];
		for (const auto &name_desc : name_desc) {
			const std::string &name_desc_str = name_desc.as<std::string>();
			if (!slist_find(&cfg->max_preferred_refresh_name_desc, slist_test_strcasecmp, name_desc_str.c_str())) {
				slist_append(&cfg->max_preferred_refresh_name_desc, strdup(name_desc_str.c_str()));
			}
		}
	}

	if (node["DISABLED"]) {
		const auto &name_desc = node["DISABLED"];
		for (const auto &name_desc : name_desc) {
			const std::string &name_desc_str = name_desc.as<std::string>();
			if (!slist_find(&cfg->disabled_name_desc, slist_test_strcasecmp, name_desc_str.c_str())) {
				slist_append(&cfg->disabled_name_desc, strdup(name_desc_str.c_str()));
			}
		}
	}
}

void cfg_emit(YAML::Emitter &e, struct Cfg *cfg) {
	if (!cfg) {
		return;
	}

	struct Cfg *cfg_def = cfg_default();

	e << YAML::BeginMap;

	if (cfg->arrange) {
		e << YAML::Key << "ARRANGE";
		e << YAML::Value << arrange_name(cfg->arrange);
	}

	if (cfg->align) {
		e << YAML::Key << "ALIGN";
		e << YAML::Value << align_name(cfg->align);
	}

	if (cfg->order_name_desc) {
		e << YAML::Key << "ORDER";
		e << YAML::BeginSeq;
		for (struct SList *i = cfg->order_name_desc; i; i = i->nex) {
			e << (char*)i->val;
		}
		e << YAML::EndSeq;
	}

	if (cfg->auto_scale) {
		e << YAML::Key << "AUTO_SCALE";
		e << YAML::Value << (cfg->auto_scale == ON);
	}

	if (cfg->user_scales) {
		e << YAML::Key << "SCALE";
		e << YAML::BeginSeq;
		for (struct SList *i = cfg->user_scales; i; i = i->nex) {
			struct UserScale *user_scale = (struct UserScale*)i->val;
			e << YAML::BeginMap;
			e << YAML::Key << "NAME_DESC";
			e << YAML::Value << user_scale->name_desc;
			e << YAML::Key << "SCALE";
			e << YAML::Value << user_scale->scale;
			e << YAML::EndMap;
		}
		e << YAML::EndSeq;
	}

	if (cfg->laptop_display_prefix) {
		e << YAML::Key << "LAPTOP_DISPLAY_PREFIX";
		e << YAML::Value << cfg->laptop_display_prefix;
	}

	if (cfg->max_preferred_refresh_name_desc) {
		e << YAML::Key << "MAX_PREFERRED_REFRESH";
		e << YAML::BeginSeq;
		for (struct SList *i = cfg->max_preferred_refresh_name_desc; i; i = i->nex) {
			e << (char*)i->val;
		}
		e << YAML::EndSeq;
	}

	if (cfg->disabled_name_desc) {
		e << YAML::Key << "DISABLED";
		e << YAML::BeginSeq;
		for (struct SList *i = cfg->disabled_name_desc; i; i = i->nex) {
			e << (char*)i->val;
		}
		e << YAML::EndSeq;
	}

	if (cfg->log_threshold) {
		e << YAML::Key << "LOG_THRESHOLD";
		e << YAML::Value << log_threshold_name(cfg->log_threshold);
	}

	e << YAML::EndMap;

	free_cfg(cfg_def);
}

void cfg_fix(struct Cfg *cfg) {
	if (!cfg) {
		return;
	}
	enum Align align = cfg->align;
	enum Arrange arrange = cfg->arrange;
	switch(arrange) {
		case COL:
			if (align != LEFT && align != MIDDLE && align != RIGHT) {
				log_warn("Ignoring invalid ALIGN %s for %s arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.", align_name(align), arrange_name(arrange));
				cfg->align = LEFT;
			}
			break;
		case ROW:
		default:
			if (align != TOP && align != MIDDLE && align != BOTTOM) {
				log_warn("Ignoring invalid ALIGN %s for %s arrange. Valid values are TOP, MIDDLE and BOTTOM. Using default TOP.", align_name(align), arrange_name(arrange));
				cfg->align = TOP;
			}
			break;
	}
}

bool cfg_parse_file(struct Cfg *cfg) {
	if (!cfg || !cfg->file_path) {
		return false;
	}

	try {
		YAML::Node node = YAML::LoadFile(cfg->file_path);
		cfg_parse_node(cfg, node);
	} catch (const std::exception &e) {
		log_error("\nparsing file %s %s", cfg->file_path, e.what());
		return false;
	}

	return true;
}

struct Cfg *cfg_merge_set(struct Cfg *to, struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = cfg_clone(to);

	struct SList *i, *f;

	// ARRANGE
	if (from->arrange) {
		merged->arrange = from->arrange;
	}

	// ALIGN
	if (from->align) {
		merged->align = from->align;
	}

	// ORDER, replace
	if (from->order_name_desc) {
		slist_free_vals(&merged->order_name_desc, NULL);
		for (i = from->order_name_desc; i; i = i->nex) {
			slist_append(&merged->order_name_desc, strdup((char*)i->val));
		}
	}

	// AUTO_SCALE
	if (from->auto_scale) {
		merged->auto_scale = from->auto_scale;
	}

	// SCALE
	struct UserScale *set_user_scale = NULL;
	struct UserScale *merged_user_scale = NULL;
	for (i = from->user_scales; i; i = i->nex) {
		set_user_scale = (struct UserScale*)i->val;
		f = slist_find(&merged->user_scales, slist_test_scale_name, set_user_scale);
		if (f) {
			merged_user_scale = (struct UserScale*)f->val;
			merged_user_scale->scale = set_user_scale->scale;
		} else {
			merged_user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));
			merged_user_scale->name_desc = strdup(set_user_scale->name_desc);
			merged_user_scale->scale = set_user_scale->scale;
			slist_append(&merged->user_scales, merged_user_scale);
		}
	}

	// DISABLED
	for (i = from->disabled_name_desc; i; i = i->nex) {
		if (!slist_find(&merged->disabled_name_desc, slist_test_strcasecmp, i->val)) {
			slist_append(&merged->disabled_name_desc, strdup((char*)i->val));
		}
	}

	return merged;
}

struct Cfg *cfg_merge_del(struct Cfg *to, struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = cfg_clone(to);

	struct SList *i;

	// SCALE
	for (i = from->user_scales; i; i = i->nex) {
		slist_remove_all_free(&merged->user_scales, slist_test_scale_name, i->val, free_user_scale);
	}

	// DISABLED
	for (i = from->disabled_name_desc; i; i = i->nex) {
		slist_remove_all_free(&merged->disabled_name_desc, slist_test_strcasecmp, i->val, NULL);
	}

	return merged;
}

struct Cfg *cfg_merge(struct Cfg *to, struct Cfg *from, enum CfgMergeType merge_type) {
	if (!to || !from || !merge_type) {
		return NULL;
	}

	struct Cfg *merged = NULL;

	switch (merge_type) {
		case SET:
			merged = cfg_merge_set(to, from);
			break;
		case DEL:
			merged = cfg_merge_del(to, from);
			break;
		default:
			break;
	}

	if (merged) {
		cfg_fix(merged);

		if (cfg_equal(merged, to)) {
			log_info("\nNo changes to make.");
			free_cfg(merged);
			merged = NULL;
		}
	}

	return merged;
}

struct Cfg *cfg_file_load() {
	bool found = false;

	struct Cfg *cfg = cfg_default();

	if (getenv("XDG_CONFIG_HOME"))
		found = cfg_resolve_paths(cfg, getenv("XDG_CONFIG_HOME"), "");
	if (!found && getenv("HOME"))
		found = cfg_resolve_paths(cfg, getenv("HOME"), "/.config");
	if (!found)
		found = cfg_resolve_paths(cfg, "/usr/local/etc", "");
	if (!found)
		found = cfg_resolve_paths(cfg, "/etc", "");

	if (found) {
		log_info("\nFound configuration file: %s", cfg->file_path);
		if (!cfg_parse_file(cfg)) {
			log_info("\nUsing default configuration:");
			struct Cfg *cfg_def = cfg_default();
			cfg_def->dir_path = cfg->dir_path ? strdup(cfg->dir_path) : NULL;
			cfg_def->file_path = cfg->file_path ? strdup(cfg->file_path) : NULL;
			cfg_def->file_name = cfg->file_name ? strdup(cfg->file_name) : NULL;
			free_cfg(cfg);
			cfg = cfg_def;
		}
	} else {
		log_info("\nNo configuration file found, using defaults:");
	}
	log_set_threshold(cfg->log_threshold, false);
	cfg_fix(cfg);
	print_cfg(cfg);

	return cfg;
}

struct Cfg *cfg_file_reload(struct Cfg *cfg) {
	if (!cfg || !cfg->file_path)
		return cfg;

	struct Cfg *cfg_new = cfg_default();
	cfg_new->dir_path = cfg->dir_path ? strdup(cfg->dir_path) : NULL;
	cfg_new->file_path = cfg->file_path ? strdup(cfg->file_path) : NULL;
	cfg_new->file_name = cfg->file_name ? strdup(cfg->file_name) : NULL;

	log_info("\nReloading configuration file: %s", cfg->file_path);
	if (cfg_parse_file(cfg_new)) {
		log_set_threshold(cfg_new->log_threshold, false);
		cfg_fix(cfg_new);
		print_cfg(cfg_new);
		free_cfg(cfg);
		return cfg_new;
	} else {
		log_info("\nConfiguration unchanged:");
		print_cfg(cfg);
		free_cfg(cfg_new);
		return cfg;
	}
}

void cfg_file_write(struct Cfg *cfg) {
	if (!cfg || !cfg->file_path) {
		log_error("\nMissing file path");
		return;
	}

	YAML::Emitter e;
	try {
		e << YAML::TrueFalseBool;
		e << YAML::UpperCase;

		cfg_emit(e, cfg);

		if (!e.good()) {
			log_error("\nWriting to file: %s", e.GetLastError().c_str());
			return;
		}

	} catch (const std::exception &e) {
		log_error("\nWriting to file: %s\n%s", e.what());
		return;
	}

	FILE *f = fopen(cfg->file_path, "w");
	if (!f) {
		log_error_errno("\nUnable to write to %s", cfg->file_path);
		return;
	}

	fprintf(f, "%s\n", e.c_str());

	fclose(f);

	cfg->written = true;
}

void free_cfg(struct Cfg *cfg) {
	if (!cfg)
		return;

	if (cfg->dir_path) {
		free(cfg->dir_path);
	}
	if (cfg->file_path) {
		free(cfg->file_path);
	}
	if (cfg->file_name) {
		free(cfg->file_name);
	}

	for (struct SList *i = cfg->order_name_desc; i; i = i->nex) {
		free(i->val);
	}
	slist_free(&cfg->order_name_desc);

	for (struct SList *i = cfg->user_scales; i; i = i->nex) {
		free_user_scale((struct UserScale*)i->val);
	}
	slist_free(&cfg->user_scales);

	for (struct SList *i = cfg->max_preferred_refresh_name_desc; i; i = i->nex) {
		free(i->val);
	}
	slist_free(&cfg->max_preferred_refresh_name_desc);

	for (struct SList *i = cfg->disabled_name_desc; i; i = i->nex) {
		free(i->val);
	}
	slist_free(&cfg->disabled_name_desc);

	if (cfg->laptop_display_prefix) {
		free(cfg->laptop_display_prefix);
	}

	free(cfg);
}

void free_user_scale(void *data) {
	struct UserScale *user_scale = (struct UserScale*)data;

	if (!user_scale)
		return;

	free(user_scale->name_desc);

	free(user_scale);
}

