#include <stdio.h>
#include <string.h>
#include <strings.h>

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
	{ .val = AUTO_SCALE,            .name = "AUTO_SCALE",            },
	{ .val = SCALE,                 .name = "SCALE",                 },
	{ .val = LAPTOP_DISPLAY_PREFIX, .name = "LAPTOP_DISPLAY_PREFIX", },
	{ .val = MAX_PREFERRED_REFRESH, .name = "MAX_PREFERRED_REFRESH", },
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

static struct NameVal auto_scales[] = {
	{ .val = ON,  .name = "ON",    },
	{ .val = OFF, .name = "OFF",   },
	{ .val = ON,  .name = "TRUE",  },
	{ .val = OFF, .name = "FALSE", },
	{ .val = ON,  .name = "YES",   },
	{ .val = OFF, .name = "NO",    },
	{ .val = 0,   .name = NULL,    },
};

static struct NameVal ipc_request_commands[] = {
	{ .val = CFG_GET,   .name = "CFG_GET",   .friendly = "get",    },
	{ .val = CFG_SET,   .name = "CFG_SET",   .friendly = "set",    },
	{ .val = CFG_DEL,   .name = "CFG_DEL",   .friendly = "delete", },
	{ .val = CFG_WRITE, .name = "CFG_WRITE", .friendly = "write",  },
	{ .val = 0,         .name = NULL,        .friendly = NULL,     },
};

static struct NameVal ipc_response_fields[] = {
	{ .val = RC,         .name = "RC",         },
	{ .val = MESSAGES,   .name = "MESSAGES",   },
	{ .val = DONE,       .name = "DONE",       },
	{ .val = 0,          .name = NULL,         },
};

static struct NameVal log_thresholds[] = {
	{ .val = DEBUG,   .name = "DEBUG",   },
	{ .val = INFO,    .name = "INFO",    },
	{ .val = WARNING, .name = "WARNING", },
	{ .val = ERROR,   .name = "ERROR",   },
	{ .val = 0,       .name = NULL,      },
};

unsigned int val(struct NameVal *name_vals, const char *name) {
	if (!name_vals || !name) {
		return 0;
	}
	for (int i = 0; name_vals[i].name; i++) {
		if (strcasecmp(name_vals[i].name, name) == 0) {
			return name_vals[i].val;
		}
	}
	return 0;
}

unsigned int val_start(struct NameVal *name_vals, const char *name) {
	if (!name_vals || !name) {
		return 0;
	}
	for (int i = 0; name_vals[i].name; i++) {
		if (strcasestr(name_vals[i].name, name) == name_vals[i].name) {
			return name_vals[i].val;
		}
	}
	return 0;
}

const char *name(struct NameVal *name_vals, unsigned int val) {
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

const char *friendly(struct NameVal *name_vals, unsigned int val) {
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

enum AutoScale auto_scale_val(const char *name) {
	return val(auto_scales, name);
}

const char *auto_scale_name(enum AutoScale auto_scale) {
	return name(auto_scales, auto_scale);
}

enum IpcRequestCommand ipc_request_command_val(const char *name) {
	return val(ipc_request_commands, name);
}

const char *ipc_request_command_name(enum IpcRequestCommand ipc_request_command) {
	return name(ipc_request_commands, ipc_request_command);
}

const char *ipc_request_command_friendly(enum IpcRequestCommand ipc_request_command) {
	return friendly(ipc_request_commands, ipc_request_command);
}

const char *ipc_response_field_name(enum IpcResponseField ipc_response_field) {
	return name(ipc_response_fields, ipc_response_field);
}

enum LogThreshold log_threshold_val(const char *name) {
	return val(log_thresholds, name);
}

const char *log_threshold_name(enum LogThreshold log_threshold) {
	return friendly(log_thresholds, log_threshold);
}

