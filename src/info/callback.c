#include <stdio.h>
#include <stdlib.h>

#include "info/callback.h"

#include "cfg.h"
#include "convert.h"
#include "head.h"
#include "log.h"
#include "mode.h"
#include "process.h"
#include "ssmap.h"
#include "str.h"

void callback(const enum LogThreshold t, const char * const msg1, const char * const msg2) {
	if (!g_cfg->callback_cmd || t < log_get_threshold()) {
		return;
	}

	log_debug(NULL);
	log_debug("Executing CALLBACK_CMD:");
	log_debug("  %s", g_cfg->callback_cmd);

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
	spawn_sh_cmd(g_cfg->callback_cmd, env);

	ssmap_free(env);
	free(buf);
}

void callback_mode_fail(const enum LogThreshold t, const struct Head * const head, const struct Mode * const mode) {
	if (!head) {
		return;
	}

	char *str = mode_str(mode);

	char *human = sprintf_alloc(
			"%s\n"
			"  Unable to set mode %s, retrying",
			head_human(head),
			str);

	callback(t, human, NULL);

	free(str);
	free(human);
}

void callback_adaptive_sync_fail(const enum LogThreshold t, const struct Head * const head) {
	if (!g_cfg->callback_cmd || !head) {
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

	callback(t, human, NULL);

	free(human);
}

