#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <wayland-client-protocol.h>

#include "cfg.h"
#include "cfg/disabled.h"
#include "convert.h"
#include "ipc.h"
#include "log.h"
#include "mode.h"
#include "process.h"
#include "pset.h"
#include "spmap.h"
#include "simap.h"
#include "sset.h"
#include "str.h"

struct Cfg *args_cfg(enum IpcCommand command, enum CfgElement element, int argc, char **argv) {
	struct Mode *mode = NULL;
	enum wl_output_transform wl_transform = 0;
	float scale = 0;

	struct Cfg *cfg = cfg_init();

	bool parsed = false;
	switch (element) {
		case ARRANGE_ALIGN:
			parsed = (cfg->arrange = arrange_val_start(argv[optind]));
			parsed = parsed && (cfg->align = align_val_start(argv[optind + 1]));
			break;
		case SCALING:
			switch (command) {
				case CFG_TOGGLE:
					cfg->scaling = ON;
					parsed = true;
					break;
				case CFG_SET:
					parsed = (cfg->scaling = on_off_val(argv[optind]));
					break;
				default:
					break;
			}
			break;
		case AUTO_SCALE:
			switch (command) {
				case CFG_TOGGLE:
					cfg->auto_scale = ON;
					parsed = true;
					break;
				case CFG_SET:
					parsed = (cfg->auto_scale = on_off_val(argv[optind]));
					break;
				default:
					break;
			}
			break;
		case SCALE:
			switch (command) {
				case CFG_SET:
					// parse input value
					parsed = ((scale = strtof(argv[optind + 1], NULL)) > 0);
					simap_put(cfg->scales, argv[optind], round(scale*1000));
					break;
				case CFG_DEL:
					// dummy value
					simap_put(cfg->scales, argv[optind], 1);
					parsed = true;
					break;
				default:
					break;
			}
			break;
		case MODE:
			switch (command) {
				case CFG_SET:
					// parse input value
					mode = mode_init();
					if (strcasecmp(argv[optind + 1], "MAX") == 0) {
						mode->max = true;
						parsed = true;
					} else {
						if (optind + 2 < argc) {
							parsed = ((mode->width = atoi(argv[optind + 1])) > 0);
							parsed = parsed && ((mode->height = atoi(argv[optind + 2])) > 0);
						}
						if (optind + 3 < argc) {
							parsed = parsed && ((mode->refresh_mhz = lround(atof(argv[optind + 3]) * 1000)) > 0);
						}
					}
					spmap_put(cfg->modes, argv[optind], mode);
					break;
				case CFG_DEL:
					// dummy value
					mode = mode_init();
					mode->max = true;
					spmap_put(cfg->modes, argv[optind], mode);
					parsed = true;
					break;
				default:
					break;
			}
			break;
		case VRR_OFF:
			for (int i = optind; i < argc; i++) {
				sset_add(cfg->adaptive_sync_off, argv[i]);
			}
			parsed = true;
			break;
		case TRANSFORM:
			switch (command) {
				case CFG_SET:
					// parse input value
					parsed = (wl_transform = transform_val(argv[optind + 1]));
					simap_put(cfg->transforms, argv[optind], wl_transform);
					break;
				case CFG_DEL:
					// dummy value
					simap_put(cfg->transforms, argv[optind], WL_OUTPUT_TRANSFORM_90);
					parsed = true;
					break;
				default:
					break;
			}
			break;
		case DISABLED:
			for (int i = optind; i < argc; i++) {
				struct Disabled *disabled = disabled_init();
				disabled->name_desc = strdup(argv[i]);
				pset_add(cfg->disableds, disabled);
			}
			parsed = true;
			break;
		case ORDER:
			for (int i = optind; i < argc; i++) {
				sset_add(cfg->order_name_desc, argv[i]);
			}
			parsed = true;
			break;
		case CALLBACK_CMD:
			switch (command) {
				case CFG_SET:
					cfg->callback_cmd = strdup(argv[optind]);
					parsed = true;
					break;
				case CFG_DEL:
					cfg->callback_cmd = strdup("");
					parsed = true;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	if (!parsed) {
		char *msg = strdup("");
		for (int i = optind; i < argc; i++) {
			msg = sprintf_append(msg, " %s", argv[i]);
		}
		log_fatal("invalid %s%s", cfg_element_name(element), msg);
		free(msg);
		cfg_free(cfg);
		wd_exit(EXIT_FAILURE);
		return NULL;
	}

	return cfg;
}

struct IpcRequest *args_ipc_list(int argc) {
	if (optind != argc) {
		log_fatal("--list takes no arguments");
		wd_exit(EXIT_FAILURE);
		return NULL;
	}

	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->command = LIST;

	return request;
}

struct IpcRequest *args_ipc_get(int argc) {
	if (optind != argc) {
		log_fatal("--get takes no arguments");
		wd_exit(EXIT_FAILURE);
		return NULL;
	}

	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->command = GET;

	return request;
}

struct IpcRequest *args_ipc_write(int argc) {
	if (optind != argc) {
		log_fatal("--write takes no arguments");
		wd_exit(EXIT_FAILURE);
		return NULL;
	}

	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->command = CFG_WRITE;

	return request;
}

struct IpcRequest *args_ipc_reapply(int argc) {
	if (optind != argc) {
		log_fatal("--reapply takes no arguments");
		wd_exit(EXIT_FAILURE);
		return NULL;
	}

	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->command = REAPPLY;

	return request;
}

struct IpcRequest *args_ipc_set(int argc, char **argv) {
	enum CfgElement element = cfg_element_val(optarg);
	switch (element) {
		case MODE:
			if (optind + 2 > argc || optind + 4 < argc) {
				log_fatal("--%s %s requires two to four arguments", ipc_command_friendly(CFG_SET), cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		case ARRANGE_ALIGN:
		case SCALE:
		case TRANSFORM:
			if (optind + 2 != argc) {
				log_fatal("--%s %s requires two arguments", ipc_command_friendly(CFG_SET), cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		case SCALING:
		case AUTO_SCALE:
		case DISABLED:
		case VRR_OFF:
		case CALLBACK_CMD:
			if (optind + 1 != argc) {
				log_fatal("--%s %s requires one argument", ipc_command_friendly(CFG_SET), cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		case ORDER:
			if (optind + 1 > argc) {
				log_fatal("--%s %s requires at least one argument", ipc_command_friendly(CFG_SET), cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		default:
			log_fatal("invalid --%s: %s", ipc_command_friendly(CFG_SET), element ? cfg_element_name(element) : optarg);
			wd_exit(EXIT_FAILURE);
			return NULL;
	}

	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->command = CFG_SET;
	request->cfg = args_cfg(CFG_SET, element, argc, argv);

	return request;
}

struct IpcRequest *args_ipc_del(int argc, char **argv) {
	enum CfgElement element = cfg_element_val(optarg);
	switch (element) {
		case MODE:
		case TRANSFORM:
		case SCALE:
		case DISABLED:
		case VRR_OFF:
			if (optind + 1 != argc) {
				log_fatal("--%s %s requires one argument", ipc_command_friendly(CFG_DEL), cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		case CALLBACK_CMD:
			if (optind != argc) {
				log_fatal("--%s %s takes no arguments", ipc_command_friendly(CFG_DEL), cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		default:
			log_fatal("invalid --%s: %s", ipc_command_friendly(CFG_DEL), element ? cfg_element_name(element) : optarg);
			wd_exit(EXIT_FAILURE);
			return NULL;
	}

	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->command = CFG_DEL;
	request->cfg = args_cfg(CFG_DEL, element, argc, argv);

	return request;
}

struct IpcRequest *args_ipc_toggle(int argc, char **argv) {
	enum CfgElement element = cfg_element_val(optarg);
	switch (element) {
		case SCALING:
		case AUTO_SCALE:
			if (optind != argc) {
				log_fatal("--%s %s takes no arguments", ipc_command_friendly(CFG_TOGGLE), cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		case VRR_OFF:
		case DISABLED:
			if (optind + 1 != argc) {
				log_fatal("--%s %s requires one argument", ipc_command_friendly(CFG_TOGGLE), cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		default:
			log_fatal("invalid --%s: %s", ipc_command_friendly(CFG_TOGGLE), element ? cfg_element_name(element) : optarg);
			wd_exit(EXIT_FAILURE);
			return NULL;
	}

	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->command = CFG_TOGGLE;
	request->cfg = args_cfg(CFG_TOGGLE, element, argc, argv);

	return request;
}

enum LogThreshold args_log_threshold(char *optarg) {
	enum LogThreshold threshold = log_threshold_val(optarg);

	if (!threshold) {
		log_fatal("invalid --log-threshold %s", optarg);
		return 0;
	}

	return threshold;
}

