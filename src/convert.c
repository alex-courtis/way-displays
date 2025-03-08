#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <wayland-client-protocol.h>

#include "convert.h"

#include "cfg.h"
#include "ipc.h"
#include "log.h"

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
	{ .val = MODE,                  .name = "MODE",                  },
	{ .val = VRR_OFF,               .name = "VRR_OFF",               },
	{ .val = CALLBACK_CMD,          .name = "CALLBACK_CMD"           },
	{ .val = LAPTOP_DISPLAY_PREFIX, .name = "LAPTOP_DISPLAY_PREFIX", },
	{ .val = MAX_PREFERRED_REFRESH, .name = "MAX_PREFERRED_REFRESH", },
	{ .val = TRANSFORM,             .name = "TRANSFORM",             },
	{ .val = LOG_THRESHOLD,         .name = "LOG_THRESHOLD",         },
	{ .val = DISABLED,              .name = "DISABLED",              },
	{ .val = ARRANGE_ALIGN,         .name = "ARRANGE_ALIGN",         },
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
	{ .val = GET,       .name = "GET",       .friendly = "get",    },
	{ .val = CFG_SET,   .name = "CFG_SET",   .friendly = "set",    },
	{ .val = CFG_DEL,   .name = "CFG_DEL",   .friendly = "delete", },
	{ .val = CFG_WRITE, .name = "CFG_WRITE", .friendly = "write",  },
	{ .val = 0,         .name = NULL,        .friendly = NULL,     },
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

static unsigned int val(struct NameVal *name_vals, const char *name) {
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

static unsigned int val_start(struct NameVal *name_vals, const char *name) {
	if (!name_vals || !name) {
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

static const char *name(struct NameVal *name_vals, unsigned int val) {
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

enum CfgElement cfg_element_val(const char *name) {
	return val(cfg_elements, name);
}

const char *cfg_element_name(enum CfgElement cfg_element) {
	return name(cfg_elements, cfg_element);
}

enum Arrange arrange_val_start(const char *name) {
	return val_start(arranges, name);
}

const char *arrange_name(enum Arrange arrange) {
	return name(arranges, arrange);
}

enum Align align_val_start(const char *name) {
	return val_start(aligns, name);
}

const char *align_name(enum Align align) {
	return name(aligns, align);
}

enum OnOff on_off_val(const char *name) {
	return val(on_offs, name);
}

const char *on_off_name(enum OnOff on_off) {
	return name(on_offs, on_off);
}

enum IpcCommand ipc_command_val(const char *name) {
	return val(ipc_commands, name);
}

const char *ipc_command_name(enum IpcCommand ipc_command) {
	return name(ipc_commands, ipc_command);
}

const char *ipc_command_friendly(enum IpcCommand ipc_command) {
	return friendly(ipc_commands, ipc_command);
}

enum wl_output_transform transform_val(const char *name) {
	return val(transforms, name);
}

const char *transform_name(enum wl_output_transform transform) {
	return name(transforms, transform);
}

enum LogThreshold log_threshold_val(const char *name) {
	return val(log_thresholds, name);
}

const char *log_threshold_name(enum LogThreshold log_threshold) {
	return friendly(log_thresholds, log_threshold);
}

