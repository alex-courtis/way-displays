#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#include <yaml-cpp/yaml.h> // IWYU pragma: keep
#include <yaml-cpp/emitter.h>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/detail/iterator.h>
#include <yaml-cpp/node/detail/iterator_fwd.h>
#include <yaml-cpp/node/impl.h>
#include <yaml-cpp/node/iterator.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <exception>
#include <stdexcept>
#include <string>

#include "marshalling.h"

extern "C" {
#include "cfg.h"
#include "convert.h"
#include "global.h"
#include "head.h"
#include "ipc.h"
#include "lid.h"
#include "log.h"
#include "mode.h"
#include "slist.h"
#include "wlr-output-management-unstable-v1.h"
}

// If this is a regex pattern, attempt to compile it before including it in configuration.
bool validate_regex(const char *pattern, enum CfgElement element) {
	bool rc = true;
	if (pattern[0] == '!') {
		regex_t regex;
		int result = regcomp(&regex, pattern + 1, REG_EXTENDED);
		if (result) {
			char err[1024];
			regerror(result, &regex, err, 1024);
			log_warn("Ignoring bad %s regex '%s':  %s", cfg_element_name(element), pattern + 1, err);
			rc = false;
		}
		regfree(&regex);
	}
	return rc;
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

char *yaml_with_newline(const YAML::Emitter &e) {
	char *yaml = (char*)calloc(e.size() + 2, sizeof(char));
	snprintf(yaml, e.size() + 2, "%s\n", e.c_str());
	return yaml;
}

/*
 * Marshal operators
 */
YAML::Emitter& operator << (YAML::Emitter& e, struct Cfg& cfg) {

	if (cfg.arrange) {
		e << YAML::Key << "ARRANGE" << YAML::Value << arrange_name(cfg.arrange);
	}

	if (cfg.align) {
		e << YAML::Key << "ALIGN" << YAML::Value << align_name(cfg.align);
	}

	if (cfg.order_name_desc) {
		e << YAML::Key << "ORDER" << YAML::BeginSeq;					// ORDER
		for (struct SList *i = cfg.order_name_desc; i; i = i->nex) {
			if (i->val) {
				e << (char*)i->val;
			}
		}
		e << YAML::EndSeq;												// ORDER
	}

	if (cfg.scaling) {
		e << YAML::Key << "SCALING" << YAML::Value << (cfg.scaling == ON);
	}

	if (cfg.auto_scale) {
		e << YAML::Key << "AUTO_SCALE" << YAML::Value << (cfg.auto_scale == ON);
	}

	if (cfg.user_scales) {
		e << YAML::Key << "SCALE" << YAML::BeginSeq;					// SCALE
		for (struct SList *i = cfg.user_scales; i; i = i->nex) {
			struct UserScale *user_scale = (struct UserScale*)i->val;
			if (user_scale) {
				e << YAML::BeginMap;											// scale
				e << YAML::Key << "NAME_DESC" << YAML::Value << user_scale->name_desc;
				e << YAML::Key << "SCALE" << YAML::Value << user_scale->scale;
				e << YAML::EndMap;												// scale
			}
		}
		e << YAML::EndSeq;												// SCALE
	}

	if (cfg.user_modes) {
		e << YAML::Key << "MODE" << YAML::BeginSeq;						// MODE
		for (struct SList *i = cfg.user_modes; i; i = i->nex) {
			struct UserMode *user_mode = (struct UserMode*)i->val;
			if (user_mode) {
				e << YAML::BeginMap;											// mode
				e << YAML::Key << "NAME_DESC" << YAML::Value << user_mode->name_desc;
				if (user_mode->max) {
					e << YAML::Key << "MAX" << YAML::Value << true;
				} else {
					e << YAML::Key << "WIDTH" << YAML::Value << user_mode->width;
					e << YAML::Key << "HEIGHT" << YAML::Value << user_mode->height;
					if (user_mode->refresh_hz != -1) {
						e << YAML::Key << "HZ" << YAML::Value << user_mode->refresh_hz;
					}
				}
				e << YAML::EndMap;												// mode
			}
		}
		e << YAML::EndSeq;												// MODE
	}

	if (cfg.adaptive_sync_off_name_desc) {
		e << YAML::Key << "VRR_OFF" << YAML::BeginSeq;					// VRR_OFF
		for (struct SList *i = cfg.adaptive_sync_off_name_desc; i; i = i->nex) {
			if (i->val) {
				e << (char*)i->val;
			}
		}
		e << YAML::EndSeq;												// VRR_OFF
	}

	if (cfg.max_preferred_refresh_name_desc) {
		e << YAML::Key << "MAX_PREFERRED_REFRESH" << YAML::BeginSeq;	// MAX_PREFERRED_REFRESH
		for (struct SList *i = cfg.max_preferred_refresh_name_desc; i; i = i->nex) {
			if (i->val) {
				e << (char*)i->val;
			}
		}
		e << YAML::EndSeq;												// MAX_PREFERRED_REFRESH
	}

	if (cfg.laptop_display_prefix) {
		e << YAML::Key << "LAPTOP_DISPLAY_PREFIX" << YAML::Value << cfg.laptop_display_prefix;
	}

	if (cfg.log_threshold) {
		e << YAML::Key << "LOG_THRESHOLD" << YAML::Value << log_threshold_name(cfg.log_threshold);
	}

	if (cfg.disabled_name_desc) {
		e << YAML::Key << "DISABLED" << YAML::BeginSeq;					// DISABLED
		for (struct SList *i = cfg.disabled_name_desc; i; i = i->nex) {
			if (i->val) {
				e << (char*)i->val;
			}
		}
		e << YAML::EndSeq;												// DISABLED
	}

	return e;
}

YAML::Emitter& operator << (YAML::Emitter& e, struct Mode& mode) {

	e << YAML::Key << "WIDTH" << YAML::Value << mode.width;
	e << YAML::Key << "HEIGHT" << YAML::Value << mode.height;
	e << YAML::Key << "REFRESH_MHZ" << YAML::Value << mode.refresh_mhz;
	e << YAML::Key << "PREFERRED" << YAML::Value << mode.preferred;

	return e;
}

YAML::Emitter& operator << (YAML::Emitter& e, struct HeadState& head_state) {

	e << YAML::Key << "SCALE" << YAML::Value << wl_fixed_to_double(head_state.scale);
	e << YAML::Key << "ENABLED" << YAML::Value << head_state.enabled;
	e << YAML::Key << "X" << YAML::Value << head_state.x;
	e << YAML::Key << "Y" << YAML::Value << head_state.y;
	e << YAML::Key << "VRR" << YAML::Value << (head_state.adaptive_sync == ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED);

	if (head_state.mode) {
		e << YAML::Key << "MODE" << YAML::BeginMap;
		e << *head_state.mode;
		e << YAML::EndMap;
	}

	return e;
}

YAML::Emitter& operator << (YAML::Emitter& e, struct Head& head) {

	if (head.name)
		e << YAML::Key << "NAME" << YAML::Value << head.name;
	if (head.description)
		e << YAML::Key << "DESCRIPTION" << YAML::Value << head.description;
	if (head.make)
		e << YAML::Key << "MAKE" << YAML::Value << head.make;
	if (head.model)
		e << YAML::Key << "MODEL" << YAML::Value << head.model;
	if (head.serial_number)
		e << YAML::Key << "SERIAL_NUMBER" << YAML::Value << head.serial_number;
	e << YAML::Key << "WIDTH_MM" << YAML::Value << head.width_mm;
	e << YAML::Key << "HEIGHT_MM" << YAML::Value << head.height_mm;
	e << YAML::Key << "TRANSFORM" << YAML::Value << head.transform;

	e << YAML::Key << "CURRENT" << YAML::BeginMap;		// CURRENT
	e << head.current;
	e << YAML::EndMap;									// CURRENT

	e << YAML::Key << "DESIRED" << YAML::BeginMap;		// DESIRED
	e << head.desired;
	e << YAML::EndMap;									// DESIRED

	if (head.modes) {
		e << YAML::Key << "MODES" << YAML::BeginSeq;	// MODES

		for (struct SList *i = head.modes; i; i = i->nex) {
			if (i->val) {
				e << YAML::BeginMap;							// mode
				e << *(Mode*)(i->val);
				e << YAML::EndMap;								// mode
			}
		}

		e << YAML::EndSeq;								// MODES
	}

	return e;
}

// marshal cfg, validating user input
void cfg_from_node_user(struct Cfg *cfg, const YAML::Node &node) {
	if (!cfg || !node || !node.IsMap()) {
		throw std::runtime_error("empty CFG");
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
			const char *order_cstr = order_str.c_str();
			if (!slist_find_equal(cfg->order_name_desc, slist_predicate_strcmp, order_cstr)) {
				if (!validate_regex(order_cstr, ORDER)) {
					continue;
				}
				slist_append(&cfg->order_name_desc, strdup(order_cstr));
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

	if (node["SCALING"]) {
		bool scaling;
		if (parse_node_val_bool(node, "SCALING", &scaling, "", "")) {
			cfg->scaling = scaling ? ON : OFF;
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
			if (!validate_regex(user_scale->name_desc, SCALE)) {
				cfg_user_mode_free(user_scale);
				continue;
			}
			if (!parse_node_val_float(scale, "SCALE", &user_scale->scale, "SCALE", user_scale->name_desc)) {
				cfg_user_scale_free(user_scale);
				continue;
			}

			slist_remove_all_free(&cfg->user_scales, cfg_equal_user_scale_name, user_scale, cfg_user_scale_free);
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
			if (!validate_regex(user_mode->name_desc, MODE)) {
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

			slist_remove_all_free(&cfg->user_modes, cfg_equal_user_mode_name, user_mode, cfg_user_mode_free);
			slist_append(&cfg->user_modes, user_mode);
		}
	}

	if (node["VRR_OFF"]) {
		const auto &offs = node["VRR_OFF"];
		for (const auto &off : offs) {
			const std::string &off_str = off.as<std::string>();
			const char *off_cstr = off_str.c_str();
			if (!slist_find_equal(cfg->adaptive_sync_off_name_desc, slist_predicate_strcmp, off_cstr)) {
				if (!validate_regex(off_cstr, VRR_OFF)) {
					continue;
				}
				slist_append(&cfg->adaptive_sync_off_name_desc, strdup(off_cstr));
			}
		}
	}

	if (node["MAX_PREFERRED_REFRESH"]) {
		const auto &maxes = node["MAX_PREFERRED_REFRESH"];
		for (const auto &max : maxes) {
			const std::string &max_str = max.as<std::string>();
			const char *max_cstr = max_str.c_str();
			if (!slist_find_equal(cfg->max_preferred_refresh_name_desc, slist_predicate_strcmp, max_cstr)) {
				if (!validate_regex(max_cstr, MAX_PREFERRED_REFRESH)) {
					continue;
				}
				slist_append(&cfg->max_preferred_refresh_name_desc, strdup(max_cstr));
			}
		}
	}

	if (node["DISABLED"]) {
		const auto &disableds = node["DISABLED"];
		for (const auto &disabled : disableds) {
			const std::string &disabled_str = disabled.as<std::string>();
			const char *disabled_cstr = disabled_str.c_str();
			if (!slist_find_equal(cfg->disabled_name_desc, slist_predicate_strcmp, disabled_cstr)) {
				if (!validate_regex(disabled_cstr, DISABLED)) {
					continue;
				}
				slist_append(&cfg->disabled_name_desc, strdup(disabled_cstr));
			}
		}
	}
}

/*
 * unmarshalling operators, ignoring failures for individual values
 */
#define TI(STATEMENT) try { STATEMENT; } catch (...) { }

struct UserScale*& operator << (struct UserScale*& user_scale, const YAML::Node& node) {
	if (!node || !node.IsMap())
		return user_scale;

	if (!user_scale)
		user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));

	TI(user_scale->name_desc = strdup(node["NAME_DESC"].as<std::string>().c_str()));
	TI(user_scale->scale = node["SCALE"].as<float>());

	return user_scale;
}

struct UserMode*& operator << (struct UserMode*& user_mode, const YAML::Node& node) {
	if (!node || !node.IsMap())
		return user_mode;

	if (!user_mode)
		user_mode = (struct UserMode*)calloc(1, sizeof(struct UserMode));

	TI(user_mode->name_desc = strdup(node["NAME_DESC"].as<std::string>().c_str()));
	TI(user_mode->width = node["WIDTH"].as<int>());
	TI(user_mode->height = node["HEIGHT"].as<int>());
	TI(user_mode->refresh_hz = node["HZ"].as<int>());
	TI(user_mode->max = node["MAX"].as<bool>());

	return user_mode;
}

struct Cfg*& operator << (struct Cfg*& cfg, const YAML::Node& node) {
	if (!node || !node.IsMap())
		return cfg;

	if (!cfg)
		cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));

	TI(cfg->arrange = arrange_val_start(node["ARRANGE"].as<std::string>().c_str()));

	TI(cfg->align = align_val_start(node["ALIGN"].as<std::string>().c_str()));

	if (node["ORDER"] && node["ORDER"].IsSequence()) {
		for (const auto &node_order : node["ORDER"]) {
			TI(slist_append(&cfg->order_name_desc, strdup(node_order.as<std::string>().c_str())));
		}
	}

	TI(cfg->scaling = node["SCALING"].as<bool>() ? ON : OFF);

	TI(cfg->auto_scale = node["AUTO_SCALE"].as<bool>() ? ON : OFF);

	if (node["SCALE"] && node["SCALE"].IsSequence()) {
		for (const auto &node_scale : node["SCALE"]) {
			struct UserScale *scale = NULL;
			if (scale << node_scale) {
				slist_append(&cfg->user_scales, scale);
			}
		}
	}

	if (node["MODE"] && node["MODE"].IsSequence()) {
		for (const auto &node_mode : node["MODE"]) {
			struct UserMode *mode = cfg_user_mode_default();
			if (mode << node_mode) {
				slist_append(&cfg->user_modes, mode);
			}
		}
	}

	if (node["VRR_OFF"] && node["VRR_OFF"].IsSequence()) {
		for (const auto &node_vrr_off : node["VRR_OFF"]) {
			TI(slist_append(&cfg->adaptive_sync_off_name_desc, strdup(node_vrr_off.as<std::string>().c_str())));
		}
	}

	TI(cfg->laptop_display_prefix = strdup(node["LAPTOP_DISPLAY_PREFIX"].as<std::string>().c_str()));

	TI(cfg->log_threshold = log_threshold_val(node["LOG_THRESHOLD"].as<std::string>().c_str()));

	if (node["DISABLED"] && node["DISABLED"].IsSequence()) {
		for (const auto &node_disabled : node["DISABLED"]) {
			TI(slist_append(&cfg->disabled_name_desc, strdup(node_disabled.as<std::string>().c_str())));
		}
	}

	return cfg;
}

struct Lid*& operator << (struct Lid*& lid, const YAML::Node& node) {
	if (!node || !node.IsMap())
		return lid;

	if (!lid)
		lid = (struct Lid*)calloc(1, sizeof(struct Lid));

	if (node["CLOSED"])
		lid->closed = node["CLOSED"].as<bool>();

	if (node["DEVICE_PATH"])
		lid->device_path = strdup(node["DEVICE_PATH"].as<std::string>().c_str());

	return lid;
}

struct Mode*& operator << (struct Mode*& mode, const YAML::Node& node) {
	if (!node || !node.IsMap())
		return mode;

	if (!mode)
		mode = (struct Mode*)calloc(1, sizeof(struct Mode));

	TI(mode->width = node["WIDTH"].as<int>());
	TI(mode->height = node["HEIGHT"].as<int>());
	TI(mode->refresh_mhz = node["REFRESH_MHZ"].as<int>());
	TI(mode->preferred = node["PREFERRED"].as<bool>());

	return mode;
}

struct HeadState& operator << (struct HeadState& head_state, const YAML::Node& node) {
	if (!node || !node.IsMap())
		return head_state;

	TI(head_state.enabled = node["ENABLED"].as<bool>());
	TI(head_state.scale = wl_fixed_from_double(node["SCALE"].as<float>()));
	TI(head_state.x = node["X"].as<int>());
	TI(head_state.y = node["Y"].as<int>());

	bool vrr = false;
	TI(vrr = node["VRR"].as<bool>());
	head_state.adaptive_sync = vrr ? ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_ENABLED : ZWLR_OUTPUT_HEAD_V1_ADAPTIVE_SYNC_STATE_DISABLED;

	head_state.mode << node["MODE"];

	return head_state;
}

struct Head*& operator << (struct Head*& head, const YAML::Node& node) {
	if (!node || !node.IsMap())
		return head;

	if (!head)
		head = (struct Head*)calloc(1, sizeof(struct Head));

	TI(head->name = strdup(node["NAME"].as<std::string>().c_str()));
	TI(head->description = strdup(node["DESCRIPTION"].as<std::string>().c_str()));
	TI(head->make = strdup(node["MAKE"].as<std::string>().c_str()));
	TI(head->model = strdup(node["MODEL"].as<std::string>().c_str()));
	TI(head->serial_number = strdup(node["SERIAL_NUMBER"].as<std::string>().c_str()));
	TI(head->width_mm = node["WIDTH_MM"].as<int>());
	TI(head->height_mm = node["HEIGHT_MM"].as<long>());
	TI(head->transform = (enum wl_output_transform)node["TRANSFORM"].as<int>());

	head->current << node["CURRENT"];
	head->desired << node["DESIRED"];

	if (node["MODES"] && node["MODES"].IsSequence()) {
		for (const auto &node_mode : node["MODES"]) {
			struct Mode *mode = NULL;
			if (mode << node_mode) {
				slist_append(&head->modes, mode);
			}
		}
	}

	return head;
}

char *marshal_ipc_request(struct IpcRequest *request) {
	if (!request) {
		return NULL;
	}

	try {
		YAML::Emitter e;

		e << YAML::TrueFalseBool;
		e << YAML::UpperCase;

		e << YAML::BeginMap;						// root

		const char *op_name = ipc_command_name(request->command);
		if (op_name) {
			e << YAML::Key << "OP" << YAML::Value << op_name;
		} else {
			log_error("marshalling ipc request: missing OP");
			return NULL;
		}

		if (request->cfg) {
			e << YAML::Key << "CFG" << YAML::BeginMap;	// CFG
			e << *request->cfg;
			e << YAML::EndMap;							// CFG
		}

		e << YAML::EndMap;							// root

		if (!e.good()) {
			log_error("marshalling ipc request: %s", e.GetLastError().c_str());
			return NULL;
		}

		return yaml_with_newline(e);

	} catch (const std::exception &e) {
		log_error("\nmarshalling ipc request: %s\n%s", e.what());
		return NULL;
	}
}

struct IpcRequest *unmarshal_ipc_request(char *yaml) {
	if (!yaml) {
		return NULL;
	}

	struct IpcRequest *request = (struct IpcRequest*)calloc(1, sizeof(struct IpcRequest));

	try {
		YAML::Node node = YAML::Load(yaml);
		if (!node.IsMap()) {
			throw std::runtime_error("empty request");
		}

		const YAML::Node node_op = node["OP"];
		if (node_op) {
			const std::string &op_str = node_op.as<std::string>();
			request->command = ipc_command_val(op_str.c_str());
			if (!request->command) {
				throw std::runtime_error("invalid OP '" + op_str + "'");
			}
		} else {
			throw std::runtime_error("missing OP");
		}

		const YAML::Node node_cfg = node["CFG"];
		if (node_cfg && node_cfg.IsMap()) {
			request->cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));
			cfg_from_node_user(request->cfg, node_cfg);
		}

		return request;

	} catch (const std::exception &e) {
		log_error("\nunmarshalling ipc request: %s", e.what());
		log_error("========================================\n%s\n----------------------------------------", yaml);
		ipc_request_free(request);
		return NULL;
	}
}

char *marshal_ipc_response(struct IpcOperation *operation) {
	char *yaml = NULL;

	try {
		YAML::Emitter e;

		e << YAML::TrueFalseBool;
		e << YAML::UpperCase;

		e << YAML::BeginSeq;							// root

		e << YAML::BeginMap;								// list

		e << YAML::Key << "DONE" << YAML::Value << operation->done;

		if (operation->send_state) {
			if (cfg) {
				e << YAML::Key << "CFG" << YAML::BeginMap;		// CFG
				e << *cfg;
				e << YAML::EndMap;								// CFG
			}

			if (lid || heads) {
				e << YAML::Key << "STATE" << YAML::BeginMap;	// STATE

				if (lid) {
					e << YAML::Key << "LID" << YAML::BeginMap;		// LID
					e << YAML::Key << "CLOSED" << YAML::Value << lid->closed;
					e << YAML::Key << "DEVICE_PATH" << YAML::Value << lid->device_path;
					e << YAML::EndMap;								// LID
				}

				if (heads) {
					e << YAML::Key << "HEADS" << YAML::BeginSeq;	// HEADS
					for (struct SList *i = heads; i; i = i->nex) {
						if (i->val) {
							e << YAML::BeginMap << *(Head*)(i->val) << YAML::EndMap;
						}
					}
					e << YAML::EndSeq;								// HEADS
				}

				e << YAML::EndMap;								// STATE
			}
		}

		if (operation->send_logs) {
			e << YAML::Key << "MESSAGES" << YAML::BeginSeq;		// MESSAGES
			for (struct SList *i = log_cap_lines; i; i = i->nex) {
				struct LogCapLine *cap_line = (struct LogCapLine*)i->val;
				if (cap_line && cap_line->line) {
					e << YAML::BeginMap;
					e << YAML::Key << log_threshold_name(cap_line->threshold);
					e << YAML::Value << cap_line->line;
					e << YAML::EndMap;
					if (cap_line->threshold == WARNING && operation->rc < IPC_RC_WARN) {
						operation->rc = IPC_RC_WARN;
					}
					if (cap_line->threshold == ERROR && operation->rc < IPC_RC_ERROR) {
						operation->rc = IPC_RC_ERROR;
					}
				}
			}
			e << YAML::EndSeq;									// MESSAGES
		}

		e << YAML::Key << "RC" << YAML::Value << operation->rc;

		e << YAML::EndMap;									// root

		e << YAML::EndSeq;

		if (!e.good()) {
			log_error("marshalling ipc response: %s", e.GetLastError().c_str());
			return NULL;
		}

		yaml = yaml_with_newline(e);

	} catch (const std::exception &e) {
		log_error("marshalling ipc response: %s\n%s", e.what());
	}

	if (operation->send_logs) {
		log_capture_clear();
	}

	return yaml;
}

void log_messages(YAML::Node node) {
	if (!node || !node.IsSequence()) {
		return;
	}

	// iterate the sequence in MESSAGES
	for (const auto &node_msg : node) {

		// each message is a map
		if (!node_msg.IsMap()) {
			continue;
		}

		// iterate the one line in the map
		for (YAML::const_iterator iter_msg = node_msg.begin(); iter_msg != node_msg.end(); ++iter_msg) {
			enum LogThreshold threshold = log_threshold_val(iter_msg->first.as<std::string>().c_str());
			if (threshold) {
				log_(threshold, "%s", iter_msg->second.as<std::string>().c_str());
			}
		}
	}
}

struct IpcResponseStatus *unmarshal_ipc_responses_print(char *yaml) {
	if (!yaml) {
		return NULL;
	}

	struct IpcResponseStatus *response_status = (struct IpcResponseStatus*)calloc(1, sizeof(struct IpcResponseStatus));
	response_status->rc = IPC_RC_BAD_RESPONSE;

	try {
		const YAML::Node node_root = YAML::Load(yaml);

		if (!node_root.IsSequence()) {
			throw std::runtime_error("empty response, expected sequence");
		}

		// iterate the unnamed list of all responses
		for (const auto &node_response : node_root) {
			if (!node_response.IsMap()) {
				throw std::runtime_error("empty entry, expected map");
			}

			// overwrite status with each response
			if (!node_response["DONE"])
				throw std::runtime_error("DONE missing");
			response_status->done = node_response["DONE"].as<bool>();

			if (!node_response["RC"])
				throw std::runtime_error("RC missing");
			response_status->rc = node_response["RC"].as<int>();

			// maybe print messages
			log_messages(node_response["MESSAGES"]);
		}

	} catch (const std::exception &e) {
		log_error("\nunmarshalling ipc response: %s", e.what());
		log_error("========================================\n%s\n----------------------------------------", yaml);
		ipc_response_status_free(response_status);
		response_status = NULL;
	}

	return response_status;
}

struct IpcResponse *unmarshal_ipc_response(char *yaml) {
	if (!yaml) {
		return NULL;
	}

	struct IpcResponse *response = (struct IpcResponse*)calloc(1, sizeof(struct IpcResponse));

	try {
		const YAML::Node node_root = YAML::Load(yaml);

		if (!node_root.IsSequence() || node_root.size() != 1) {
			throw std::runtime_error("invalid response");
		}

		const YAML::Node node = node_root[0];
		if (!node.IsMap()) {
			throw std::runtime_error("invalid response");
		}

		if (!node["DONE"]) {
			throw std::runtime_error("DONE missing");
		}

		if (!node["RC"]) {
			throw std::runtime_error("RC missing");
		}

		response->cfg << node["CFG"];

		if (node["STATE"]) {
			response->lid << node["STATE"]["LID"];

			if (node["STATE"]["HEADS"] && node["STATE"]["HEADS"].IsSequence()) {
				for (const auto &node_head : node["STATE"]["HEADS"]) {
					struct Head *head = NULL;
					slist_append(&response->heads, head << node_head);
				}
			}
		}

		for (YAML::const_iterator i = node.begin(); i != node.end(); ++i) {

			if (i->first.as<std::string>() == "DONE") {
				response->status.done = i->second.as<bool>();
			}

			if (i->first.as<std::string>() == "RC") {
				response->status.rc = i->second.as<int>();
			}

			if (i->first.as<std::string>() == "MESSAGES" && i->second.IsSequence()) {
				for (YAML::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
					if (j->IsMap()) {
						for (YAML::const_iterator k = j->begin(); k != j->end(); ++k) {
							enum LogThreshold threshold = log_threshold_val(k->first.as<std::string>().c_str());
							if (threshold) {
								log_(threshold, "%s", k->second.as<std::string>().c_str());
							}
						}
					}
				}
			}
		}

	} catch (const std::exception &e) {
		log_error("\nunmarshalling ipc response: %s", e.what());
		log_error("========================================\n%s\n----------------------------------------", yaml);
		ipc_response_free(response);
		response = NULL;
	}

	return response;
}

char *marshal_cfg(struct Cfg *cfg) {
	if (!cfg) {
		return NULL;
	}

	try {
		YAML::Emitter e;

		e << YAML::TrueFalseBool;
		e << YAML::UpperCase;

		e << YAML::BeginMap;	// root
		e << *cfg;
		e << YAML::EndMap;		// root

		if (!e.good()) {
			log_error("marshalling cfg: %s", e.GetLastError().c_str());
			return NULL;
		}

		return yaml_with_newline(e);

	} catch (const std::exception &e) {
		log_error("marshalling cfg request: %s\n%s", e.what());
		return NULL;
	}
}

bool unmarshal_cfg_from_file(struct Cfg *cfg) {
	if (!cfg->file_path) {
		return false;
	}

	try {
		YAML::Node node = YAML::LoadFile(cfg->file_path);
		cfg_from_node_user(cfg, node);
	} catch (const std::exception &e) {
		log_error("\nparsing file %s %s", cfg->file_path, e.what());
		return false;
	}

	return true;
}

