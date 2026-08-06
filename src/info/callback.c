#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "info/callback.h"

#include "cfg/cfg.h"
#include "enum.h"
#include "head.h"
#include "log.h"
#include "mode.h"
#include "ppmap.h"
#include "process.h"
#include "ssmap.h"
#include "str.h"
#include "wlr-output-management-unstable-v1.h"

// true if the callback command is a non-empty string
static bool callback_cmd_valid(const struct Cfg * const cfg) {
	return cfg && cfg->callback_cmd && strlen(cfg->callback_cmd) > 0;
}

void callback_with_cfg(const enum LogThreshold t, const struct Cfg * const cfg, const char * const msg1, const char * const msg2) {
	if (!callback_cmd_valid(cfg)) {
		return;
	}

	enum LogThreshold t_cur = log_get_threshold();
	if (t < t_cur) {
		return;
	}

	log_debug(NULL);
	log_debug("Executing CALLBACK_CMD:");
	log_debug("  %s", cfg->callback_cmd);

	// decorate human message and optional log
	char *buf = (char*)calloc(CALLBACK_MSG_LEN, sizeof(char));
	snprintf(buf, CALLBACK_MSG_LEN, "%s%s", msg1 ? msg1 : "", msg2 ? msg2 : "");

	// pack environment variables
	const struct SSmap *env = ssmap_init();

	ssmap_put_if_absent(env, "CALLBACK_MSG", buf);
	ssmap_put_if_absent(env, "CALLBACK_LEVEL", log_threshold_name(t));

	char *env_str = ssmap_str(env);
	log_debug("%s", env_str);
	free(env_str);

	// execute callback
	spawn_sh_cmd(cfg->callback_cmd, env);

	ssmap_free(env);
	free(buf);
}

void callback(const enum LogThreshold t, const char * const msg1, const char * const msg2) {
	callback_with_cfg(t, g_cfg, msg1, msg2);
}

void callback_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct zwlr_output_mode_v1* const zmode) {
	if (!callback_cmd_valid(g_cfg) || !head || !zmode) {
		return;
	}

	const struct Mode *mode = ppmap_get(head->modes, zmode);
	char *str = mode_str_pref(mode, head->zmode_pref == zmode);

	char *human = sprintf_alloc(
			"%s\n"
			"  Unable to set mode %s, retrying",
			head_human(head),
			str);

	callback_with_cfg(t, g_cfg, human, NULL);

	free(str);
	free(human);
}

void callback_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head) {
	if (!callback_cmd_valid(g_cfg) || !head) {
		return;
	}

	// custom human message
	char *human = sprintf_alloc(
			"%s\n"
			"  Cannot enable VRR.\n"
			"  You can disable VRR for this display in cfg.yaml\n"
			"VRR_OFF:\n"
			"  - '%s'",
			head_human(head),
			head->model ? head->model : "name_desc");

	callback_with_cfg(t, g_cfg, human, NULL);

	free(human);
}

