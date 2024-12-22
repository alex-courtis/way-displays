#ifndef GLOBAL_H
#define GLOBAL_H

extern struct Displ *displ;
extern struct Cfg *cfg;
extern struct Lid *lid;

extern struct SList *cfg_file_paths;

// layout change in progress
// single operations for individual heads are sometimes set
struct LayoutDelta {
	struct Head *head_mode;
	struct Head *head_adaptive_sync;
	char *brief;
};
extern struct LayoutDelta layout_delta;

#endif // GLOBAL_H
