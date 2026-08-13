#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "args.h"
#include "enum.h"
#include "ipc.h"
#include "log.h"
#include "process.h"

#include "cli.h"

void cli_usage(FILE *stream) {
	const char mesg[] =
		"Usage: way-displays [OPTIONS...] [COMMAND]\n"
		"  Runs the server when no COMMAND specified.\n"
		"OPTIONS\n"
		"  -L, --lo[g-threshold] <debug|info|warning|error>\n"
		"  -c, --c[onfig]        <path>\n"
		"  -y, --y[aml]          YAML client output, implies -L warning\n"
		"COMMANDS\n"
		"  -h, --h[elp]    show this message\n"
		"  -v, --v[ersion] display version information\n"
		"  -l, --li[st]    list connected\n"
		"  -g, --g[et]     show config and state\n"
		"  -w, --w[rite]   write active to cfg.yaml\n"
		"  -r, --r[eapply] disable, reset failed modes, enable\n"
		"  -s, --s[et]     add or change\n"
		"     ARRANGE_ALIGN <row|column> <top|middle|bottom|left|right>\n"
		"     ORDER <name> ...\n"
		"     SCALING <on|off>\n"
		"     AUTO_SCALE <on|off>\n"
		"     SCALE <name> <scale>\n"
		"     MODE <name> MAX\n"
		"     MODE <name> <width> <height> [<Hz>]\n"
		"     TRANSFORM <name> <90|180|270|flipped|flipped-90|flipped-180|flipped-270>\n"
		"     DISABLED <name>\n"
		"     VRR_OFF <name>\n"
		"     CALLBACK_CMD <shell command>\n"
		"  -t, --t[toggle] toggle parameter\n"
		"     SCALING\n"
		"     AUTO_SCALE\n"
		"     DISABLED <name>\n"
		"     VRR_OFF <name>\n"
		"  -d, --d[elete]  remove\n"
		"     SCALE <name>\n"
		"     MODE <name>\n"
		"     TRANSFORM <name>\n"
		"     DISABLED <name>\n"
		"     VRR_OFF <name>\n"
		"     CALLBACK_CMD\n"
		;
	fprintf(stream, "%s", mesg);
}

void cli_parse(int argc, char **argv, struct IpcRequest **ipc_request, char **cfg_path) {
	static struct option long_options[] = {
		{ "config",        required_argument, 0, 'c' },
		{ "delete",        required_argument, 0, 'd' },
		{ "get",           no_argument,       0, 'g' },
		{ "help",          no_argument,       0, 'h' },
		{ "list",          no_argument,       0, 'l' },
		{ "log-threshold", required_argument, 0, 'L' },
		{ "reapply",       no_argument,       0, 'r' },
		{ "set",           required_argument, 0, 's' },
		{ "toggle",        required_argument, 0, 't' },
		{ "version",       no_argument,       0, 'v' },
		{ "write",         no_argument,       0, 'w' },
		{ "yaml",          no_argument,       0, 'y' },
		{ 0,               0,                 0,  0  }
	};
	static char *short_options = "c:d:ghlL:rs:t:vwy";

	bool yaml = false;
	enum LogThreshold threshold = 0;

	while (1) {
		int long_index = 0;
		int c = getopt_long(argc, argv, short_options, long_options, &long_index);
		if (c == -1)
			break;
		switch (c) {
			case 'L':
				if (!(threshold = args_log_threshold(optarg))) {
					wd_exit(EXIT_FAILURE);
					return;
				}
				break;
			case 'h':
				cli_usage(stdout);
				wd_exit(EXIT_SUCCESS);
				return;
			case 'c':
				*cfg_path = strdup(optarg);
				break;
			case 'v':
				log_info("way-displays version %s %s", VERSION, COMMIT);
				wd_exit(EXIT_SUCCESS);
				break;
			case 'y':
				yaml = true;
				break;
			case 'l':
				*ipc_request = args_ipc_list(argc);
				break;
			case 'g':
				*ipc_request = args_ipc_get(argc);
				break;
			case 's':
				*ipc_request = args_ipc_set(argc, argv);
				break;
			case 'd':
				*ipc_request = args_ipc_del(argc, argv);
				break;
			case 't':
				*ipc_request = args_ipc_toggle(argc, argv);
				break;
			case 'w':
				*ipc_request = args_ipc_write(argc);
				break;
			case 'r':
				*ipc_request = args_ipc_reapply(argc);
				break;
			case '?':
			default:
				cli_usage(stderr);
				wd_exit(EXIT_FAILURE);
				return;
		}
	}

	log_set_threshold(threshold, true);

	if (*ipc_request) {
		if (((*ipc_request)->yaml |= yaml)) {
			(*ipc_request)->log_threshold = WARNING;
		} else {
			(*ipc_request)->log_threshold = threshold;
		}
	}
}
