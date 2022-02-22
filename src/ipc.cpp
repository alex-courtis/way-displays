#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h> // IWYU pragma: keep
#include <yaml-cpp/emitter.h>
#include <yaml-cpp/emittermanip.h>
#include <yaml-cpp/node/detail/iterator.h>
#include <yaml-cpp/node/detail/iterator_fwd.h>
#include <yaml-cpp/node/impl.h>
#include <yaml-cpp/node/iterator.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <exception>
#include <stdexcept>
#include <string>

#include "ipc.h"

#include "cfg.h"

extern "C" {
#include "convert.h"
#include "info.h"
#include "list.h"
#include "log.h"
#include "sockets.h"
}

char *yaml_with_newline(YAML::Emitter &e) {
	char *yaml = (char*)calloc(e.size() + 2, sizeof(char));
	snprintf(yaml, e.size() + 2, "%s\n", e.c_str());
	return yaml;
}

char *marshal_request(struct IpcRequest *request) {
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

struct IpcRequest *unmarshal_request(char *yaml) {
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

char *marshal_response(struct IpcResponse *response) {
	char *yaml = NULL;

	log_capture_end();

	try {
		YAML::Emitter e;

		e << YAML::TrueFalseBool;
		e << YAML::UpperCase;

		e << YAML::BeginMap;

		e << YAML::Key << ipc_response_field_name(DONE);
		e << YAML::Value << response->done;

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

		e << YAML::Key << ipc_response_field_name(RC);
		e << YAML::Value << response->rc;

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

struct IpcResponse *unmarshal_response(char *yaml) {
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
				for (YAML::const_iterator j = i->second.begin(); j != i->second.end(); j++) {
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

int ipc_request_send(struct IpcRequest *request) {
	int fd = -1;

	char *yaml = marshal_request(request);
	if (!yaml) {
		goto end;
	}

	log_debug("========sending server request==========\n%s\n----------------------------------------", yaml);
	log_info("Sending %s request:", ipc_request_command_friendly(request->command));
	print_cfg(request->cfg);

	if ((fd = create_fd_ipc_client()) == -1) {
		goto end;
	}

	if (socket_write(fd, yaml, strlen(yaml)) == -1) {
		fd = -1;
		goto end;
	}

end:
	if (yaml) {
		free(yaml);
	}

	return fd;
}

void ipc_response_send(struct IpcResponse *response) {
	char *yaml = marshal_response(response);

	if (!yaml) {
		return;
	}

	log_debug("========sending client response==========\n%s----------------------------------------", yaml);

	if (socket_write(response->fd, yaml, strlen(yaml)) == -1) {
		response->done = true;
	}

	free(yaml);

	if (response->done) {
		close(response->fd);
	} else {
		log_capture_start();
	}
}

struct IpcRequest *ipc_request_receive(int fd_sock) {
	struct IpcRequest *request = NULL;
	char *yaml = NULL;
	int fd = -1;

	if ((fd = socket_accept(fd_sock)) == -1) {
		return NULL;
	}

	if (!(yaml = socket_read(fd))) {
		close(fd);
		return NULL;
	}

	log_debug("========received client request=========\n%s\n----------------------------------------", yaml);

	log_capture_start();

	request = unmarshal_request(yaml);
	free(yaml);

	log_capture_end();

	if (!request) {
		request = (struct IpcRequest*)calloc(1, sizeof(struct IpcRequest));
		request->bad = true;
		request->fd = fd;
		return request;
	}

	request->fd = fd;

	return request;
}

struct IpcResponse *ipc_response_receive(int fd) {
	struct IpcResponse *response = NULL;
	char *yaml = NULL;

	if (fd == -1) {
		log_error("invalid fd for ipc response receive");
		goto err;
	}

	if (!(yaml = socket_read(fd))) {
		goto err;
	}

	log_debug("========received server response========\n%s\n----------------------------------------", yaml);

	response = unmarshal_response(yaml);
	free(yaml);

	return response;

err:
	log_error("\nFailed to read IPC response");

	response = (struct IpcResponse*)calloc(1, sizeof(struct IpcResponse));
	response->done = true;
	response->rc = 1;

	return response;
}

void free_ipc_request(struct IpcRequest *request) {
	if (!request) {
		return;
	}

	free_cfg(request->cfg);

	free(request);
}

void free_ipc_response(struct IpcResponse *response) {
	if (!response) {
		return;
	}

	free(response);
}

