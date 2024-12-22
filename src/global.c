#include <stddef.h>

#include "global.h"

struct Displ *displ = NULL;
struct Lid *lid = NULL;
struct Cfg *cfg = NULL;

struct SList *cfg_file_paths = NULL;

struct LayoutDelta layout_delta = {
	.head_mode = NULL,
	.head_adaptive_sync = NULL,
	.brief = NULL,
};

