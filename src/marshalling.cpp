#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "ipc.h"
#include "list.h"
#include "log.h"
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

char *yaml_with_newline(YAML::Emitter &e) {
	char *yaml = (char*)calloc(e.size() + 2, sizeof(char));
	snprintf(yaml, e.size() + 2, "%s\n", e.c_str());
	return yaml;
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

char *marshal_ipc_request(struct IpcRequest *request) {
	if (!request) {
		return NULL;
	}

	try {
		YAML::Emitter e;

		e << YAML::TrueFalseBool;
		e << YAML::UpperCase;

		e << YAML::BeginMap;

		e << YAML::Key << ipc_request_command_name(request->command);

		if (request->cfg) {
			cfg_emit(e, request->cfg);
		} else {
			e << YAML::Value << "";
		}

		e << YAML::EndMap;

		if (!e.good()) {
			log_error("marshalling ipc request: %s", e.GetLastError().c_str());
			return NULL;
		}

		return yaml_with_newline(e);

	} catch (const std::exception &e) {
		log_error("marshalling ipc request: %s\n%s", e.what());
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
			throw std::runtime_error("no commands");
		}

		if (node.size() != 1) {
			throw std::runtime_error("multiple commands");
		}

		request->command = ipc_request_command_val(node.begin()->first.as<std::string>().c_str());
		if (!request->command) {
			throw std::runtime_error("invalid command");
		}

		switch (request->command) {
			case CFG_SET:
			case CFG_DEL:
				request->cfg = (struct Cfg*)calloc(1, sizeof(struct Cfg));
				cfg_parse_node(request->cfg, node.begin()->second);
				break;
			case CFG_GET:
			default:
				break;
		}

		return request;

	} catch (const std::exception &e) {
		log_error("unmarshalling ipc request: %s\n========================================\n%s\n----------------------------------------", e.what(), yaml);
		free_ipc_request(request);
		return NULL;
	}
}

char *marshal_ipc_response(struct IpcResponse *response) {
	char *yaml = NULL;

	log_capture_end();

	try {
		YAML::Emitter e;

		e << YAML::TrueFalseBool;
		e << YAML::UpperCase;

		e << YAML::BeginMap;

		e << YAML::Key << ipc_response_field_name(DONE);
		e << YAML::Value << response->done;

		e << YAML::Key << ipc_response_field_name(RC);
		e << YAML::Value << response->rc;

		if (response->cfg && response->done) {
			e << YAML::Key << ipc_response_field_name(CFG);
			cfg_emit(e, response->cfg);
		}

		if (log_cap_lines) {

			e << YAML::Key << ipc_response_field_name(MESSAGES);

			e << YAML::BeginMap;

			for (struct SList *i = log_cap_lines; i; i = i->nex) {
				struct LogCapLine *cap_line = (struct LogCapLine*)i->val;
				if (cap_line && cap_line->line) {
					e << YAML::Key << log_threshold_name(cap_line->threshold);
					e << YAML::Value << cap_line->line;
					if (cap_line->threshold == ERROR) {
						response->rc = EXIT_FAILURE;
					}
				}
			}

			e << YAML::EndMap;
		}

		e << YAML::EndMap;

		if (!e.good()) {
			log_error("marshalling ipc response: %s", e.GetLastError().c_str());
			return NULL;
		}

		yaml = yaml_with_newline(e);

	} catch (const std::exception &e) {
		log_error("marshalling ipc response: %s\n%s", e.what());
	}

	log_capture_reset();

	return yaml;
}

struct IpcResponse *unmarshal_ipc_response(char *yaml) {
	struct IpcResponse *response = (struct IpcResponse*)calloc(1, sizeof(struct IpcResponse));
	response->rc = EXIT_FAILURE;
	response->done = true;

	if (!yaml) {
		return response;
	}

	try {
		const YAML::Node node = YAML::Load(yaml);

		if (!node.IsMap()) {
			throw std::runtime_error("invalid response");
		}

		if (!node["DONE"]) {
			throw std::runtime_error("DONE missing");
		}

		if (!node["RC"]) {
			throw std::runtime_error("RC missing");
		}

		for (YAML::const_iterator i = node.begin(); i != node.end(); ++i) {

			if (i->first.as<std::string>() == "DONE") {
				response->done = i->second.as<bool>();
			}

			if (i->first.as<std::string>() == "RC") {
				response->rc = i->second.as<int>();
			}

			if (i->first.as<std::string>() == "MESSAGES" && i->second.IsMap()) {
				for (YAML::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
					enum LogThreshold threshold = log_threshold_val(j->first.as<std::string>().c_str());
					if (threshold) {
						log_(threshold, "%s", j->second.as<std::string>().c_str());
					}
				}
			}
		}

	} catch (const std::exception &e) {
		log_error("unmarshalling ipc response: %s\n========================================\n%s\n----------------------------------------", e.what(), yaml);
		response->rc = EXIT_FAILURE;
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

		cfg_emit(e, cfg);

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
		cfg_parse_node(cfg, node);
	} catch (const std::exception &e) {
		log_error("\nparsing file %s %s", cfg->file_path, e.what());
		return false;
	}

	return true;
}

