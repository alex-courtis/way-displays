#ifndef CFG_DISABLED_H
#define CFG_DISABLED_H

struct Disabled {
	char *name_desc;
	const struct PSet *conditions;
};

struct Disabled *disabled_init(void);

struct Disabled *disabled_init_name_desc(const char *name_desc);

const struct PSet *disabled_pset_init(void);

const struct Disabled *disabled_clone(const struct Disabled * const from);

void disabled_free(struct Disabled *disabled);

#endif // CFG_DISABLED_H
