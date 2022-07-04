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
#include "server.h"
}

bool parse_node_val_bool(const YAML::Node &node, const char *key, bool *val, const char *desc1, const char *desc2) {
	if (node[key]) {
		try {
			*val = node[key].as<bool>();
		} catch (YAML::BadConversion &e) {
			log_warn("Ignoring invalid %s %s %s %s", desc1, desc2, key, node[key].as<std::string>().c_str());
			return false;
		}
	} else {
		log_warn("Ignoring missing %s %s %s", desc1, desc2, key);
		return false;
	}
	return true;
}

bool parse_node_val_string(const YAML::Node &node, const char *key, char **val, const char *desc1, const char *desc2) {
	if (node[key]) {
		try {
			*val = strdup(node[key].as<std::string>().c_str());
		} catch (YAML::BadConversion &e) {
			log_warn("Ignoring invalid %s %s %s %s", desc1, desc2, key, node[key].as<std::string>().c_str());
			return false;
		}
	} else {
		log_warn("Ignoring missing %s %s %s", desc1, desc2, key);
		return false;
	}
	return true;
}

bool parse_node_val_int(const YAML::Node &node, const char *key, int *val, const char *desc1, const char *desc2) {
	if (node[key]) {
		try {
			*val = node[key].as<int>();
		} catch (YAML::BadConversion &e) {
			log_warn("Ignoring invalid %s %s %s %s", desc1, desc2, key, node[key].as<std::string>().c_str());
			return false;
		}
	} else {
		log_warn("Ignoring missing %s %s %s", desc1, desc2, key);
		return false;
	}
	return true;
}

bool parse_node_val_float(const YAML::Node &node, const char *key, float *val, const char *desc1, const char *desc2) {
	if (node[key]) {
		try {
			*val = node[key].as<float>();
		} catch (YAML::BadConversion &e) {
			log_warn("Ignoring invalid %s %s %s %s", desc1, desc2, key, node[key].as<std::string>().c_str());
			return false;
		}
	} else {
		log_warn("Ignoring missing %s %s %s", desc1, desc2, key);
		return false;
	}
	return true;
}

bool equal_user_mode_name(const void *value, const void *data) {
	if (!value || !data) {
		return false;
	}

	struct UserMode *lhs = (struct UserMode*)value;
	struct UserMode *rhs = (struct UserMode*)data;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcasecmp(lhs->name_desc, rhs->name_desc) == 0;
}

bool equal_user_transform_name(const void *value, const void *data) {
	if (!value || !data) {
		return false;
	}

	struct UserTransform *lhs = (struct UserTransform*)value;
	struct UserTransform *rhs = (struct UserTransform*)data;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	return strcasecmp(lhs->name_desc, rhs->name_desc) == 0;
}

bool equal_user_scale_name(const void *value, const void *data) {
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

bool equal_user_scale(const void *value, const void *data) {
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

bool equal_user_mode(const void *value, const void *data) {
	if (!value || !data) {
		return false;
	}

	struct UserMode *lhs = (struct UserMode*)value;
	struct UserMode *rhs = (struct UserMode*)data;

	if (!lhs->name_desc || !rhs->name_desc) {
		return false;
	}

	if (strcasecmp(lhs->name_desc, rhs->name_desc) != 0) {
		return false;
	}

	if (lhs->max != rhs->max) {
		return false;
	}

	if (lhs->width != rhs->width || lhs->height != rhs->height) {
		return false;
	}

	if ((lhs->refresh_hz != -1 || rhs->refresh_hz != -1) && lhs->refresh_hz != rhs->refresh_hz) {
		return false;
	}

	return true;
}

bool equal_user_transform(const void *value, const void *data) {
	if (!value || !data) {
		return false;
	}

	struct UserTransform *lhs = (struct UserTransform*)value;
	struct UserTransform *rhs = (struct UserTransform*)data;

	if (!lhs->transform || !rhs->transform) {
		return false;
	}

	return true;
}

bool invalid_user_scale(const void *value, const void *data) {
	if (!value) {
		return true;
	}

	struct UserScale *user_scale = (struct UserScale*)value;

	if (user_scale->scale <= 0) {
		log_warn("\nIgnoring non-positive SCALE %s %.3f", user_scale->name_desc, user_scale->scale);
		return true;
	}

	return false;
}

bool invalid_user_mode(const void *value, const void *data) {
	if (!value) {
		return true;
	}
	struct UserMode *user_mode = (struct UserMode*)value;

	if (user_mode->width != -1 && user_mode->width <= 0) {
		log_warn("\nIgnoring non-positive MODE %s WIDTH %d", user_mode->name_desc, user_mode->width);
		return true;
	}
	if (user_mode->height != -1 && user_mode->height <= 0) {
		log_warn("\nIgnoring non-positive MODE %s HEIGHT %d", user_mode->name_desc, user_mode->height);
		return true;
	}
	if (user_mode->refresh_hz != -1 && user_mode->refresh_hz <= 0) {
		log_warn("\nIgnoring non-positive MODE %s HZ %d", user_mode->name_desc, user_mode->refresh_hz);
		return true;
	}

	if (!user_mode->max) {
		if (user_mode->width == -1) {
			log_warn("\nIgnoring invalid MODE %s missing WIDTH", user_mode->name_desc);
			return true;
		}
		if (user_mode->height == -1) {
			log_warn("\nIgnoring invalid MODE %s missing HEIGHT", user_mode->name_desc);
			return true;
		}
	}

	return false;
}

void warn_short_name_desc(const char *name_desc, const char *element) {
	if (!name_desc)
		return;

	if (strlen(name_desc) < 4) {
		log_warn("\n%s '%s' is less than 4 characters, which may result in some unwanted matches.", element, name_desc);
	}
}

struct Cfg *clone_cfg(struct Cfg *from) {
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

	// MODE
	for (i = from->user_modes; i; i = i->nex) {
		struct UserMode *from_user_mode = (struct UserMode*)i->val;
		struct UserMode *to_user_mode = (struct UserMode*)calloc(1, sizeof(struct UserMode));
		to_user_mode->name_desc = strdup(from_user_mode->name_desc);
		to_user_mode->max = from_user_mode->max;
		to_user_mode->width = from_user_mode->width;
		to_user_mode->height = from_user_mode->height;
		to_user_mode->refresh_hz = from_user_mode->refresh_hz;
		to_user_mode->warned_no_mode = from_user_mode->warned_no_mode;
		slist_append(&to->user_modes, to_user_mode);
	}

	// TRANSFORM
	for (i = from->user_transform; i; i = i->nex) {
		struct UserTransform *from_user_transform = (struct UserTransform*)i->val;
		struct UserTransform *to_user_transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));
		to_user_transform->name_desc = strdup(from_user_transform->name_desc);
		to_user_transform->transform = from_user_transform->transform;
		slist_append(&to->user_transform, to_user_transform);
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

bool equal_cfg(struct Cfg *a, struct Cfg* b) {
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
	if (!slist_equal(a->order_name_desc, b->order_name_desc, slist_equal_strcasecmp)) {
		return false;
	}

	// AUTO_SCALE
	if (a->auto_scale != b->auto_scale) {
		return false;
	}

	// SCALE
	if (!slist_equal(a->user_scales, b->user_scales, equal_user_scale)) {
		return false;
	}

	// MODE
	if (!slist_equal(a->user_modes, b->user_modes, equal_user_mode)) {
		return false;
	}

	// TRANSFORM
	if (!slist_equal(a->user_transform, b->user_transform, equal_user_transform)) {
		return false;
	}

	// LAPTOP_DISPLAY_PREFIX
	char *al = a->laptop_display_prefix;
	char *bl = b->laptop_display_prefix;
	if ((al && !bl) || (!al && bl) || (al && bl && strcasecmp(al, bl) != 0)) {
		return false;
	}

	// MAX_PREFERRED_REFRESH
	if (!slist_equal(a->max_preferred_refresh_name_desc, b->max_preferred_refresh_name_desc, slist_equal_strcasecmp)) {
		return false;
	}

	// DISABLED
	if (!slist_equal(a->disabled_name_desc, b->disabled_name_desc, slist_equal_strcasecmp)) {
		return false;
	}

	// LOG_THRESHOLD
	if (a->log_threshold != b->log_threshold) {
		return false;
	}

	return true;
}

struct Cfg *cfg_default() {
	struct Cfg *def = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	def->arrange = ARRANGE_DEFAULT;
	def->align = ALIGN_DEFAULT;
	def->auto_scale = AUTO_SCALE_DEFAULT;

	return def;
}

struct UserMode *cfg_user_mode_default() {
	struct UserMode *user_mode = (struct UserMode*)calloc(1, sizeof(struct UserMode));

	user_mode->width = -1;
	user_mode->height = -1;
	user_mode->refresh_hz = -1;

	return user_mode;
}

struct UserTransform *cfg_user_transform_default() {
	struct UserTransform *user_transform = (struct UserTransform*)calloc(1, sizeof(struct UserTransform));

	user_transform->transform = WL_OUTPUT_TRANSFORM_NORMAL;

	return user_transform;
}

bool resolve_paths(struct Cfg *cfg, const char *prefix, const char *suffix) {
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
			if (!slist_find_equal(cfg->order_name_desc, slist_equal_strcasecmp, order_str.c_str())) {
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
		bool auto_scale;
		if (parse_node_val_bool(node, "AUTO_SCALE", &auto_scale, "", "")) {
			cfg->auto_scale = auto_scale ? ON : OFF;
		}
	}

	if (node["SCALE"]) {
		for (const auto &scale : node["SCALE"]) {
			struct UserScale *user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));

			if (!parse_node_val_string(scale, "NAME_DESC", &user_scale->name_desc, "SCALE", "")) {
				cfg_user_scale_free(user_scale);
				continue;
			}
			if (!parse_node_val_float(scale, "SCALE", &user_scale->scale, "SCALE", user_scale->name_desc)) {
				cfg_user_scale_free(user_scale);
				continue;
			}

			slist_remove_all_free(&cfg->user_scales, equal_user_scale_name, user_scale, cfg_user_scale_free);
			slist_append(&cfg->user_scales, user_scale);
		}
	}

	if (node["MODE"]) {
		for (const auto &mode : node["MODE"]) {
			struct UserMode *user_mode = cfg_user_mode_default();

			if (!parse_node_val_string(mode, "NAME_DESC", &user_mode->name_desc, "MODE", "")) {
				cfg_user_mode_free(user_mode);
				continue;
			}
			if (mode["MAX"] && !parse_node_val_bool(mode, "MAX", &user_mode->max, "MODE", user_mode->name_desc)) {
				cfg_user_mode_free(user_mode);
				continue;
			}
			if (mode["WIDTH"] && !parse_node_val_int(mode, "WIDTH", &user_mode->width, "MODE", user_mode->name_desc)) {
				cfg_user_mode_free(user_mode);
				continue;
			}
			if (mode["HEIGHT"] && !parse_node_val_int(mode, "HEIGHT", &user_mode->height, "MODE", user_mode->name_desc)) {
				cfg_user_mode_free(user_mode);
				continue;
			}
			if (mode["HZ"] && !parse_node_val_int(mode, "HZ", &user_mode->refresh_hz, "MODE", user_mode->name_desc)) {
				cfg_user_mode_free(user_mode);
				continue;
			}

			slist_remove_all_free(&cfg->user_modes, equal_user_mode_name, user_mode, cfg_user_mode_free);
			slist_append(&cfg->user_modes, user_mode);
		}
	}

	if (node["TRANSFORM"]) {
		for (const auto &transform : node["TRANSFORM"]) {
			struct UserTransform *user_transform = cfg_user_transform_default();

			if (!parse_node_val_string(transform, "NAME_DESC", &user_transform->name_desc, "TRANSFORM", "")) {
				cfg_user_transform_free(user_transform);
				continue;
			}
			if (!parse_node_val_int(transform, "DEGREE", (int *) &user_transform->transform, "TRANSFORM", user_transform->name_desc)) {
				cfg_user_transform_free(user_transform);
				continue;
			}

			slist_remove_all_free(&cfg->user_transform, equal_user_transform_name, user_transform, cfg_user_transform_free);
			slist_append(&cfg->user_transform, user_transform);
		}
	}

	if (node["MAX_PREFERRED_REFRESH"]) {
		const auto &name_desc = node["MAX_PREFERRED_REFRESH"];
		for (const auto &name_desc : name_desc) {
			const std::string &name_desc_str = name_desc.as<std::string>();
			if (!slist_find_equal(cfg->max_preferred_refresh_name_desc, slist_equal_strcasecmp, name_desc_str.c_str())) {
				slist_append(&cfg->max_preferred_refresh_name_desc, strdup(name_desc_str.c_str()));
			}
		}
	}

	if (node["DISABLED"]) {
		const auto &name_desc = node["DISABLED"];
		for (const auto &name_desc : name_desc) {
			const std::string &name_desc_str = name_desc.as<std::string>();
			if (!slist_find_equal(cfg->disabled_name_desc, slist_equal_strcasecmp, name_desc_str.c_str())) {
				slist_append(&cfg->disabled_name_desc, strdup(name_desc_str.c_str()));
			}
		}
	}
}

void cfg_emit(YAML::Emitter &e, struct Cfg *cfg) {
	if (!cfg) {
		return;
	}

	struct Cfg *def = cfg_default();

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

	if (cfg->user_modes) {
		e << YAML::Key << "MODE";
		e << YAML::BeginSeq;
		for (struct SList *i = cfg->user_modes; i; i = i->nex) {
			struct UserMode *user_mode = (struct UserMode*)i->val;
			e << YAML::BeginMap;
			e << YAML::Key << "NAME_DESC";
			e << YAML::Value << user_mode->name_desc;

			if (user_mode->max) {
				e << YAML::Key << "MAX";
				e << YAML::Value << true;
			} else {
				e << YAML::Key << "WIDTH";
				e << YAML::Value << user_mode->width;
				e << YAML::Key << "HEIGHT";
				e << YAML::Value << user_mode->height;
				if (user_mode->refresh_hz != -1) {
					e << YAML::Key << "HZ";
					e << YAML::Value << user_mode->refresh_hz;
				}
			}
			e << YAML::EndMap;
		}
		e << YAML::EndSeq;
	}

	if (cfg->user_transform) {
		e << YAML::Key << "TRANSFORM";
		e << YAML::BeginSeq;
		for (struct SList *i = cfg->user_modes; i; i = i->nex) {
			struct UserTransform *user_transform = (struct UserTransform*)i->val;
			e << YAML::BeginMap;
			e << YAML::Key << "NAME_DESC";
			e << YAML::Value << user_transform->name_desc;

			e << YAML::Key << "DEGREE";
			e << YAML::Value << user_transform->transform;
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

	cfg_free(def);
}

void validate_fix(struct Cfg *cfg) {
	if (!cfg) {
		return;
	}
	enum Align align = cfg->align;
	enum Arrange arrange = cfg->arrange;
	switch(arrange) {
		case COL:
			if (align != LEFT && align != MIDDLE && align != RIGHT) {
				log_warn("\nIgnoring invalid ALIGN %s for %s arrange. Valid values are LEFT, MIDDLE and RIGHT. Using default LEFT.", align_name(align), arrange_name(arrange));
				cfg->align = LEFT;
			}
			break;
		case ROW:
		default:
			if (align != TOP && align != MIDDLE && align != BOTTOM) {
				log_warn("\nIgnoring invalid ALIGN %s for %s arrange. Valid values are TOP, MIDDLE and BOTTOM. Using default TOP.", align_name(align), arrange_name(arrange));
				cfg->align = TOP;
			}
			break;
	}

	slist_remove_all_free(&cfg->user_scales, invalid_user_scale, NULL, cfg_user_scale_free);

	slist_remove_all_free(&cfg->user_modes, invalid_user_mode, NULL, cfg_user_mode_free);
}

void validate_warn(struct Cfg *cfg) {
	if (!cfg)
		return;

	struct SList *i = NULL;

	for (i = cfg->user_scales; i; i = i->nex) {
		if (!i->val)
			continue;
		struct UserScale *user_scale = (struct UserScale*)i->val;
		warn_short_name_desc(user_scale->name_desc, "SCALE");
	}
	for (i = cfg->user_modes; i; i = i->nex) {
		if (!i->val)
			continue;
		struct UserMode *user_mode = (struct UserMode*)i->val;
		warn_short_name_desc(user_mode->name_desc, "MODE");
	}
	for (i = cfg->user_transform; i; i = i->nex) {
		if (!i->val)
			continue;
		struct UserTransform *user_transform = (struct UserTransform*) i->val;
		warn_short_name_desc(user_transform->name_desc, "TRANSFORM");
	}
	for (i = cfg->order_name_desc; i; i = i->nex) {
		if (!i->val)
			continue;
		warn_short_name_desc((const char*)i->val, "ORDER");
	}
	for (i = cfg->max_preferred_refresh_name_desc; i; i = i->nex) {
		if (!i->val)
			continue;
		warn_short_name_desc((const char*)i->val, "MAX_PREFERRED_REFRESH");
	}
	for (i = cfg->disabled_name_desc; i; i = i->nex) {
		if (!i->val)
			continue;
		warn_short_name_desc((const char*)i->val, "DISABLED");
	}
}

bool parse_file(struct Cfg *cfg) {
	if (!cfg->file_path) {
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


struct Cfg *merge_set(struct Cfg *to, struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = clone_cfg(to);

	struct SList *i;

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
		if (!(merged_user_scale = (struct UserScale*)slist_find_equal_val(merged->user_scales, equal_user_scale_name, set_user_scale))) {
			merged_user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));
			merged_user_scale->name_desc = strdup(set_user_scale->name_desc);
			slist_append(&merged->user_scales, merged_user_scale);
		}
		merged_user_scale->scale = set_user_scale->scale;
	}

	// MODE
	struct UserMode *set_user_mode = NULL;
	struct UserMode *merged_user_mode = NULL;
	for (i = from->user_modes; i; i = i->nex) {
		set_user_mode = (struct UserMode*)i->val;
		if (!(merged_user_mode = (struct UserMode*)slist_find_equal_val(merged->user_modes, equal_user_mode_name, set_user_mode))) {
			merged_user_mode = cfg_user_mode_default();
			merged_user_mode->name_desc = strdup(set_user_mode->name_desc);
			slist_append(&merged->user_modes, merged_user_mode);
		}
		merged_user_mode->max = set_user_mode->max;
		merged_user_mode->width = set_user_mode->width;
		merged_user_mode->height = set_user_mode->height;
		merged_user_mode->refresh_hz = set_user_mode->refresh_hz;
		merged_user_mode->warned_no_mode = set_user_mode->warned_no_mode;
	}

	// TRANSFORM
	struct UserTransform *set_user_transform = NULL;
	struct UserTransform *merged_user_transform = NULL;
	for (i = from->user_transform; i; i = i->nex) {
		set_user_transform = (struct UserTransform*)i->val;
		if (!(merged_user_transform = (struct UserTransform*)slist_find_equal_val(merged->user_transform, equal_user_transform_name, set_user_transform))) {
			merged_user_transform = cfg_user_transform_default();
			merged_user_transform->name_desc = strdup(set_user_transform->name_desc);
			slist_append(&merged->user_modes, merged_user_transform);
		}
		merged_user_transform->transform = set_user_transform->transform;
	}

	// DISABLED
	for (i = from->disabled_name_desc; i; i = i->nex) {
		if (!slist_find_equal(merged->disabled_name_desc, slist_equal_strcasecmp, i->val)) {
			slist_append(&merged->disabled_name_desc, strdup((char*)i->val));
		}
	}

	return merged;
}

struct Cfg *merge_del(struct Cfg *to, struct Cfg *from) {
	if (!to || !from) {
		return NULL;
	}

	struct Cfg *merged = clone_cfg(to);

	struct SList *i;

	// SCALE
	for (i = from->user_scales; i; i = i->nex) {
		slist_remove_all_free(&merged->user_scales, equal_user_scale_name, i->val, cfg_user_scale_free);
	}

	// MODE
	for (i = from->user_modes; i; i = i->nex) {
		slist_remove_all_free(&merged->user_modes, equal_user_mode_name, i->val, cfg_user_mode_free);
	}

	// TRANSFORM
	for (i = from->user_transform; i; i = i->nex) {
		slist_remove_all_free(&merged->user_transform, equal_user_transform_name, i->val, cfg_user_transform_free);
	}

	// DISABLED
	for (i = from->disabled_name_desc; i; i = i->nex) {
		slist_remove_all_free(&merged->disabled_name_desc, slist_equal_strcasecmp, i->val, NULL);
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
			merged = merge_set(to, from);
			break;
		case DEL:
			merged = merge_del(to, from);
			break;
		default:
			break;
	}

	if (merged) {
		validate_fix(merged);
		validate_warn(merged);

		if (equal_cfg(merged, to)) {
			log_info("\nNo changes to make.");
			cfg_free(merged);
			merged = NULL;
		}
	}

	return merged;
}

void cfg_init(void) {
	bool found = false;

	cfg = cfg_default();

	if (getenv("XDG_CONFIG_HOME"))
		found = resolve_paths(cfg, getenv("XDG_CONFIG_HOME"), "");
	if (!found && getenv("HOME"))
		found = resolve_paths(cfg, getenv("HOME"), "/.config");
	if (!found)
		found = resolve_paths(cfg, "/usr/local/etc", "");
	if (!found)
		found = resolve_paths(cfg, "/etc", "");

	if (found) {
		log_info("\nFound configuration file: %s", cfg->file_path);
		if (!parse_file(cfg)) {
			log_info("\nUsing default configuration:");
			struct Cfg *def = cfg_default();
			def->dir_path = cfg->dir_path ? strdup(cfg->dir_path) : NULL;
			def->file_path = cfg->file_path ? strdup(cfg->file_path) : NULL;
			def->file_name = cfg->file_name ? strdup(cfg->file_name) : NULL;
			cfg_free(cfg);
			cfg = def;
		}
	} else {
		log_info("\nNo configuration file found, using defaults:");
	}
	validate_fix(cfg);
	print_cfg(INFO, cfg, false);
	validate_warn(cfg);
}

void cfg_file_reload(void) {
	if (!cfg->file_path)
		return;

	struct Cfg *reloaded = cfg_default();
	reloaded->dir_path = cfg->dir_path ? strdup(cfg->dir_path) : NULL;
	reloaded->file_path = cfg->file_path ? strdup(cfg->file_path) : NULL;
	reloaded->file_name = cfg->file_name ? strdup(cfg->file_name) : NULL;

	log_info("\nReloading configuration file: %s", cfg->file_path);
	if (parse_file(reloaded)) {
		cfg_free(cfg);
		cfg = reloaded;
		log_set_threshold(cfg->log_threshold, false);
		validate_fix(cfg);
		print_cfg(INFO, cfg, false);
		validate_warn(cfg);
	} else {
		log_info("\nConfiguration unchanged:");
		print_cfg(INFO, cfg, false);
		cfg_free(reloaded);
	}
}

void cfg_file_write(void) {
	if (!cfg->file_path) {
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

void cfg_destroy(void) {
	cfg_free(cfg);
	cfg = NULL;
}

void cfg_free(struct Cfg *cfg) {
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
		cfg_user_scale_free((struct UserScale*)i->val);
	}
	slist_free(&cfg->user_scales);

	for (struct SList *i = cfg->user_modes; i; i = i->nex) {
		cfg_user_mode_free((struct UserMode*)i->val);
	}
	slist_free(&cfg->user_modes);

	for (struct SList *i = cfg->user_transform; i; i = i->nex) {
		cfg_user_transform_free((struct UserTransform*)i->val);
	}
	slist_free(&cfg->user_transform);

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

void cfg_user_scale_free(void *data) {
	struct UserScale *user_scale = (struct UserScale*)data;

	if (!user_scale)
		return;

	free(user_scale->name_desc);

	free(user_scale);
}

void cfg_user_mode_free(void *data) {
	struct UserMode *user_mode = (struct UserMode*)data;

	if (!user_mode)
		return;

	free(user_mode->name_desc);

	free(user_mode);
}

void cfg_user_transform_free(void *data) {
	struct UserTransform *user_transform = (struct UserTransform*)data;

	if (!user_transform)
		return;

	free(user_transform->name_desc);
	free(user_transform);
}

