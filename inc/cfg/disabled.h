#ifndef CFG_DISABLED_H
#define CFG_DISABLED_H

struct Disabled {
	char *name_desc;
	struct SList *conditions;
};

struct Disabled *disabled_init_always(const char *name_desc);

const struct PSet *disabled_pset_init(void);

void* disabled_clone(const void *data);

void disabled_free(const void *val);

#endif // CFG_DISABLED_H
