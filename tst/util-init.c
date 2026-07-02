#include <stdbool.h>
#include <string.h>

#include "util-init.h"

#include "head.h"
#include "mode.h"

struct Head *head_init_name(const char *name) {
	struct Head *head = head_init();
	head->name = strdup(name);
	return head;
}

struct Head *head_init_description(const char *description) {
	struct Head *head = head_init();
	head->description = strdup(description);
	return head;
}

struct WlrMode *wlr_mode_init_empty(void) {
	return wlr_mode_init(NULL, NULL, 0, 0, 0, false);
}

struct WlrMode *wlr_mode_init_head(struct Head *head) {
	return wlr_mode_init(head, NULL, 0, 0, 0, false);
}
