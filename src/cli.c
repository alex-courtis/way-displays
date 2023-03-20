#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "cfg.h"
#include "convert.h"
#include "ipc.h"
#include "list.h"
#include "log.h"
#include "process.h"

#include "cli.h"

void usage(FILE *stream) {
	static char mesg[] =
		"Usage: way-displays [OPTIONS...] [COMMAND]\n"
		"  Runs the server when no COMMAND specified.\n"
		"OPTIONS\n"
		"  -L, --l[og-threshold] <debug|info|warning|error>\n"
		"  -c, --c[onfig]        <path>\n"
		"  -y, --y[aml]          YAML client output\n"
		"COMMANDS\n"
		"  -h, --h[elp]    show this message\n"
		"  -v, --v[ersion] display version information\n"
		"  -g, --g[et]     show the active settings\n"
		"  -w, --w[rite]   write active to cfg.yaml\n"
		"  -s, --s[et]     add or change\n"
		"     ARRANGE_ALIGN <row|column> <top|middle|bottom|left|right>\n"
		"     ORDER <name> ...\n"
		"     AUTO_SCALE <on|off>\n"
		"     SCALE <name> <scale>\n"
		"     MODE <name> MAX\n"
		"     MODE <name> <width> <height> [<Hz>]\n"
		"     DISABLED <name>\n"
		"  -d, --d[elete]  remove\n"
		"     SCALE <name>\n"
		"     MODE <name>\n"
		"     DISABLED <name>\n"
		;
	fprintf(stream, "%s", mesg);
}

struct Cfg *parse_element(enum IpcRequestOperation op, enum CfgElement element, int argc, char **argv) {
	struct UserScale *user_scale = NULL;
	struct UserMode *user_mode = NULL;

	struct Cfg *cfg = calloc(1, sizeof(struct Cfg));

	bool parsed = false;
	switch (element) {
		case ARRANGE_ALIGN:
			parsed = (cfg->arrange = arrange_val_start(argv[optind]));
			parsed = parsed && (cfg->align = align_val_start(argv[optind + 1]));
			break;
		case AUTO_SCALE:
			parsed = (cfg->auto_scale = auto_scale_val(argv[optind]));
			break;
		case SCALE:
			switch (op) {
				case CFG_SET:
					// parse input value
					user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));
					user_scale->name_desc = strdup(argv[optind]);
					parsed = ((user_scale->scale = strtof(argv[optind + 1], NULL)) > 0);
					slist_append(&cfg->user_scales, user_scale);
					break;
				case CFG_DEL:
					// dummy value
					user_scale = (struct UserScale*)calloc(1, sizeof(struct UserScale));
					user_scale->name_desc = strdup(argv[optind]);
					user_scale->scale = 1;
					slist_append(&cfg->user_scales, user_scale);
					parsed = true;
					break;
				default:
					break;
			}
			break;
		case MODE:
			switch (op) {
				case CFG_SET:
					// parse input value
					user_mode = cfg_user_mode_default();
					user_mode->name_desc = strdup(argv[optind]);
					if (strcasecmp(argv[optind + 1], "MAX") == 0) {
						user_mode->max = true;
						parsed = true;
					} else {
						if (optind + 2 < argc) {
							parsed = ((user_mode->width = atoi(argv[optind + 1])) > 0);
							parsed = parsed && ((user_mode->height = atoi(argv[optind + 2])) > 0);
						}
						if (optind + 3 < argc) {
							parsed = parsed && ((user_mode->refresh_hz = atoi(argv[optind + 3])) > 0);
						}
					}
					slist_append(&cfg->user_modes, user_mode);
					break;
				case CFG_DEL:
					// dummy value
					user_mode = cfg_user_mode_default();
					user_mode->name_desc = strdup(argv[optind]);
					user_mode->max = true;
					slist_append(&cfg->user_modes, user_mode);
					parsed = true;
					break;
				default:
					break;
			}
			break;
		case DISABLED:
			for (int i = optind; i < argc; i++) {
				slist_append(&cfg->disabled_name_desc, strdup(argv[i]));
			}
			parsed = true;
			break;
		case ORDER:
			for (int i = optind; i < argc; i++) {
				slist_append(&cfg->order_name_desc, strdup(argv[i]));
			}
			parsed = true;
			break;
		default:
			break;
	}

	if (!parsed) {
		char buf[256];
		char *bp = buf;
		for (int i = optind; i < argc; i++) {
			bp += snprintf(bp, sizeof(buf) - (bp - buf), " %s", argv[i]);
		}
		log_error("invalid %s%s", cfg_element_name(element), buf);
		if (cfg) {
			cfg_free(cfg);
		}
		wd_exit(EXIT_FAILURE);
		return NULL;
	}

	return cfg;
}

struct IpcRequest *parse_get(int argc, char **argv) {
	if (optind != argc) {
		log_error("--get takes no arguments");
		wd_exit(EXIT_FAILURE);
		return NULL;
	}

	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->op = GET;

	return request;
}

struct IpcRequest *parse_write(int argc, char **argv) {
	if (optind != argc) {
		log_error("--write takes no arguments");
		wd_exit(EXIT_FAILURE);
		return NULL;
	}

	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->op = CFG_WRITE;

	return request;
}

struct IpcRequest *parse_set(int argc, char **argv) {
	enum CfgElement element = cfg_element_val(optarg);
	switch (element) {
		case MODE:
			if (optind + 2 > argc || optind + 4 < argc) {
				log_error("%s requires two to four arguments", cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		case ARRANGE_ALIGN:
		case SCALE:
			if (optind + 2 != argc) {
				log_error("%s requires two arguments", cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		case AUTO_SCALE:
		case DISABLED:
			if (optind + 1 != argc) {
				log_error("%s requires one argument", cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		case ORDER:
			if (optind + 1 > argc) {
				log_error("%s requires at least one argument", cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		default:
			log_error("invalid %s: %s", ipc_request_op_friendly(CFG_SET), element ? cfg_element_name(element) : optarg);
			wd_exit(EXIT_FAILURE);
			return NULL;
	}

	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->op = CFG_SET;
	request->cfg = parse_element(CFG_SET, element, argc, argv);

	return request;
}

struct IpcRequest *parse_del(int argc, char **argv) {
	enum CfgElement element = cfg_element_val(optarg);
	switch (element) {
		case MODE:
		case SCALE:
		case DISABLED:
			if (optind + 1 != argc) {
				log_error("%s requires one argument", cfg_element_name(element));
				wd_exit(EXIT_FAILURE);
				return NULL;
			}
			break;
		default:
			log_error("invalid %s: %s", ipc_request_op_friendly(CFG_DEL), element ? cfg_element_name(element) : optarg);
			wd_exit(EXIT_FAILURE);
			return NULL;
	}

	struct IpcRequest *request = calloc(1, sizeof(struct IpcRequest));
	request->op = CFG_DEL;
	request->cfg = parse_element(CFG_DEL, element, argc, argv);

	return request;
}

bool parse_log_threshold(char *optarg) {
	enum LogThreshold threshold = log_threshold_val(optarg);

	if (!threshold) {
		log_error("invalid --log-threshold %s", optarg);
		return false;
	}

	log_set_threshold(threshold, true);

	return true;
}

void parse_args(int argc, char **argv, struct IpcRequest **ipc_request, char **cfg_path, bool *yaml) {
	static struct option long_options[] = {
		{ "config",        required_argument, 0, 'c' },
		{ "delete",        required_argument, 0, 'd' },
		{ "get",           no_argument,       0, 'g' },
		{ "help",          no_argument,       0, 'h' },
		{ "log-threshold", required_argument, 0, 'L' },
		{ "set",           required_argument, 0, 's' },
		{ "version",       no_argument,       0, 'v' },
		{ "write",         no_argument,       0, 'w' },
		{ "yaml",          no_argument,       0, 'y' },
		{ 0,               0,                 0,  0  }
	};
	static char *short_options = "c:d:ghL:s:vwy";

	int c;
	while (1) {
		int long_index = 0;
		c = getopt_long(argc, argv, short_options, long_options, &long_index);
		if (c == -1)
			break;
		switch (c) {
			case 'L':
				if (!parse_log_threshold(optarg)) {
					wd_exit(EXIT_FAILURE);
					return;
				}
				break;
			case 'h':
				usage(stdout);
				wd_exit(EXIT_SUCCESS);
				return;
			case 'c':
				*cfg_path = strdup(optarg);
				break;
			case 'v':
				log_info("way-displays version %s", VERSION);
				wd_exit(EXIT_SUCCESS);
				return;
			case 'y':
				*yaml = true;
				break;
			case 'g':
				*ipc_request = parse_get(argc, argv);
				return;
			case 's':
				*ipc_request = parse_set(argc, argv);
				return;
			case 'd':
				*ipc_request = parse_del(argc, argv);
				return;
			case 'w':
				*ipc_request = parse_write(argc, argv);
				return;
			case '?':
			default:
				usage(stderr);
				wd_exit(EXIT_FAILURE);
				return;
		}
	}
}

