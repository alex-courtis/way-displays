#include <string.h>
#include <strings.h>
#include <wayland-client-protocol.h>

#include "convert.h"

#include "cfg.h"
#include "cfg/condition.h"
#include "displ.h"
#include "ipc.h"
#include "log.h"
#include "str.h"

struct NameVal {
	unsigned int val;
	char *name;
	char *friendly;
};

static struct NameVal cfg_elements[] = {
	{ .val = ARRANGE,               .name = "ARRANGE",               },
	{ .val = ALIGN,                 .name = "ALIGN",                 },
	{ .val = ORDER,                 .name = "ORDER",                 },
	{ .val = SCALING,               .name = "SCALING",               },
	{ .val = AUTO_SCALE,            .name = "AUTO_SCALE",            },
	{ .val = SCALE,                 .name = "SCALE",                 },
	{ .val = SCALE_ROUND_TO,        .name = "SCALE_ROUND_TO"         },
	{ .val = SCALE_ROUND_STRATEGY,  .name = "SCALE_ROUND_STRATEGY"   },
	{ .val = MODE,                  .name = "MODE",                  },
	{ .val = VRR_OFF,               .name = "VRR_OFF",               },
	{ .val = CALLBACK_CMD,          .name = "CALLBACK_CMD"           },
	{ .val = LAPTOP_DISPLAY_PREFIX, .name = "LAPTOP_DISPLAY_PREFIX", },
	{ .val = LAPTOP_LID_MONITOR,    .name = "LAPTOP_LID_MONITOR",    },
	{ .val = MAX_PREFERRED_REFRESH, .name = "MAX_PREFERRED_REFRESH", },
	{ .val = TRANSFORM,             .name = "TRANSFORM",             },
	{ .val = LOG_THRESHOLD,         .name = "LOG_THRESHOLD",         },
	{ .val = DISABLED,              .name = "DISABLED",              },
	{ .val = ARRANGE_ALIGN,         .name = "ARRANGE_ALIGN",         },
	{ .val = AUTO_SCALE_DPI,        .name = "AUTO_SCALE_DPI",        },
	{ .val = AUTO_SCALE_MIN,        .name = "AUTO_SCALE_MIN",        },
	{ .val = AUTO_SCALE_MAX,        .name = "AUTO_SCALE_MAX",        },
	{ .val = CHANGE_SUCCESS_CMD,    .name = "CHANGE_SUCCESS_CMD",    },
	{ .val = 0,                     .name = NULL,                    },
};

static struct NameVal arranges[] = {
	{ .val = ROW, .name = "ROW",    },
	{ .val = COL, .name = "COLUMN", },
	{ .val = 0,   .name = NULL,     },
};

static struct NameVal aligns[] = {
	{ .val = TOP,    .name = "TOP",    },
	{ .val = MIDDLE, .name = "MIDDLE", },
	{ .val = BOTTOM, .name = "BOTTOM", },
	{ .val = LEFT,   .name = "LEFT",   },
	{ .val = RIGHT,  .name = "RIGHT",  },
	{ .val = 0,      .name = NULL,     },
};

static struct NameVal on_offs[] = {
	{ .val = ON,  .name = "ON",    },
	{ .val = OFF, .name = "OFF",   },
	{ .val = ON,  .name = "TRUE",  },
	{ .val = OFF, .name = "FALSE", },
	{ .val = ON,  .name = "YES",   },
	{ .val = OFF, .name = "NO",    },
	{ .val = 0,   .name = NULL,    },
};

static struct NameVal ipc_commands[] = {
	{ .val = GET,        .name = "GET",        .friendly = "get",     },
	{ .val = LIST,       .name = "LIST",       .friendly = "list",    },
	{ .val = REAPPLY,    .name = "REAPPLY",    .friendly = "reapply", },
	{ .val = CFG_SET,    .name = "CFG_SET",    .friendly = "set",     },
	{ .val = CFG_DEL,    .name = "CFG_DEL",    .friendly = "delete",  },
	{ .val = CFG_WRITE,  .name = "CFG_WRITE",  .friendly = "write",   },
	{ .val = CFG_TOGGLE, .name = "CFG_TOGGLE", .friendly = "toggle",  },
	{ .val = 0,          .name = NULL,         .friendly = NULL,      },
};

static struct NameVal transforms[] = {
	{ .val = WL_OUTPUT_TRANSFORM_90,          .name = "90",          },
	{ .val = WL_OUTPUT_TRANSFORM_180,         .name = "180",         },
	{ .val = WL_OUTPUT_TRANSFORM_270,         .name = "270",         },
	{ .val = WL_OUTPUT_TRANSFORM_FLIPPED,     .name = "FLIPPED",     },
	{ .val = WL_OUTPUT_TRANSFORM_FLIPPED_90,  .name = "FLIPPED-90",  },
	{ .val = WL_OUTPUT_TRANSFORM_FLIPPED_180, .name = "FLIPPED-180", },
	{ .val = WL_OUTPUT_TRANSFORM_FLIPPED_270, .name = "FLIPPED-270", },
	{ .val = 0,                               .name = NULL,          },
};

static struct NameVal log_thresholds[] = {
	{ .val = DEBUG,   .name = "DEBUG",   },
	{ .val = INFO,    .name = "INFO",    },
	{ .val = WARNING, .name = "WARNING", },
	{ .val = ERROR,   .name = "ERROR",   },
	{ .val = FATAL,   .name = "FATAL",   },
	{ .val = 0,       .name = NULL,      },
};

static struct NameVal displ_states[] = {
	{ .val = IDLE,        .name = "IDLE",        },
	{ .val = SUCCEEDED,   .name = "SUCCEEDED",   },
	{ .val = OUTSTANDING, .name = "OUTSTANDING", },
	{ .val = CANCELLED,   .name = "CANCELLED",   },
	{ .val = FAILED,      .name = "FAILED",      },
	{ .val = 0,           .name = NULL,          },
};

static struct NameVal condition_lids[] = {
	{ .val = LID_OPEN,        .name = "OPEN",        },
	{ .val = LID_CLOSED,      .name = "CLOSED",      },
	{ .val = LID_NOT_PRESENT, .name = "NOT_PRESENT", },
	{ .val = 0,               .name = NULL,          },
};

static struct NameVal scale_round_strategies[] = {
	{ .val = NEAREST, .name = "NEAREST", },
	{ .val = UP,      .name = "UP",      },
	{ .val = DOWN,    .name = "DOWN",    },
	{ .val = 0,       .name = NULL,      },
};

static struct NameVal scale_round_tos[] = {
	{ .val = 8, .name = "0.125", },
	{ .val = 4, .name = "0.25", },
	{ .val = 2, .name = "0.5", },
	{ .val = 1, .name = "1", },
	{ .val = 0, .name = NULL, },
};

static unsigned int _val(struct NameVal *name_vals, const char *name) {
	if (!name_vals || !name) {
		return 0;
	}
	int i;
	for (i = 0; name_vals[i].name; i++) {
		if (strcasecmp(name_vals[i].name, name) == 0) {
			return name_vals[i].val;
		}
	}
	return name_vals[i].val;
}

static unsigned int _val_start(struct NameVal *name_vals, const char *name) {
	if (!name_vals || !name || strlen(name) == 0) {
		return 0;
	}
	int i;
	for (i = 0; name_vals[i].name; i++) {
		if (strcasestr(name_vals[i].name, name) == name_vals[i].name) {
			return name_vals[i].val;
		}
	}
	return name_vals[i].val;
}

static const char *_name(struct NameVal *name_vals, unsigned int val) {
	if (!name_vals) {
		return NULL;
	}
	for (int i = 0; name_vals[i].name; i++) {
		if (val == name_vals[i].val) {
			return name_vals[i].name;
		}
	}
	return NULL;
}

static const char *friendly(struct NameVal *name_vals, unsigned int val) {
	if (!name_vals) {
		return NULL;
	}
	for (int i = 0; name_vals[i].name; i++) {
		if (val == name_vals[i].val) {
			if (name_vals[i].friendly) {
				return name_vals[i].friendly;
			} else {
				return name_vals[i].name;
			}
		}
	}
	return NULL;
}

static char *names(struct NameVal *name_vals) {
	if (!name_vals) {
		return NULL;
	}
	char *vals = NULL;
	for (int i = 0; name_vals[i].name; i++) {
		vals = sprintf_append(vals, "%s%s", i > 0 ? "|" : "", name_vals[i].name);
	}
	return vals;
}

enum CfgElement cfg_element_val(const char *name) {
	return _val(cfg_elements, name);
}

const char *cfg_element_name(enum CfgElement cfg_element) {
	return _name(cfg_elements, cfg_element);
}

enum Arrange arrange_val_start(const char *name) {
	return _val_start(arranges, name);
}

const char *arrange_name(enum Arrange arrange) {
	return _name(arranges, arrange);
}

char *arrange_names(void) {
	return names(arranges);
}

enum Align align_val_start(const char *name) {
	return _val_start(aligns, name);
}

const char *align_name(enum Align align) {
	return _name(aligns, align);
}

char *align_names(void) {
	return names(aligns);
}

enum OnOff on_off_val(const char *name) {
	return _val(on_offs, name);
}

const char *on_off_name(enum OnOff on_off) {
	return _name(on_offs, on_off);
}

char *on_off_names(void) {
	return names(on_offs);
}

enum IpcCommand ipc_command_val(const char *name) {
	return _val(ipc_commands, name);
}

const char *ipc_command_name(enum IpcCommand ipc_command) {
	return _name(ipc_commands, ipc_command);
}

char *ipc_command_names(void) {
	return names(ipc_commands);
}

const char *ipc_command_friendly(enum IpcCommand ipc_command) {
	return friendly(ipc_commands, ipc_command);
}

enum wl_output_transform transform_val(const char *name) {
	return _val(transforms, name);
}

const char *transform_name(enum wl_output_transform transform) {
	return _name(transforms, transform);
}

char *transform_names(void) {
	return names(transforms);
}

enum LogThreshold log_threshold_val(const char *name) {
	return _val(log_thresholds, name);
}

const char *log_threshold_name(enum LogThreshold log_threshold) {
	return _name(log_thresholds, log_threshold);
}

char *log_threshold_names(void) {
	return names(log_thresholds);
}

enum DisplState displ_state_val(const char *name) {
	return _val(displ_states, name);
}

const char *displ_state_name(enum DisplState displ_state) {
	return _name(displ_states, displ_state);
}

enum ConditionLid condition_lid_val(const char *name) {
	return _val(condition_lids, name);
}

const char *condition_lid_name(enum ConditionLid condition_lid) {
	return _name(condition_lids, condition_lid);
}

char *condition_lid_names(void) {
	return names(condition_lids);
}

enum ScaleRoundStrategy scale_round_strategy_val(const char *name) {
	return _val(scale_round_strategies, name);
}

const char *scale_round_strategy_name(enum ScaleRoundStrategy scale_round_strategy) {
	return _name(scale_round_strategies, scale_round_strategy);
}

char *scale_round_strategy_names(void) {
	return names(scale_round_strategies);
}

unsigned int scale_round_to_val(const float scale_round_to) {
	if (scale_round_to == 0) {
		return 0;
	}

	unsigned int ret = 1.0f / scale_round_to;

	if (!_name(scale_round_tos, ret)) {
		return 0;
	}

	return ret;
}

const char *scale_round_to_name(const unsigned int scale_round_to) {
	return _name(scale_round_tos, scale_round_to);
}

char *scale_round_to_names(void) {
	return names(scale_round_tos);
}

