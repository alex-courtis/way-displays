#include <cmocka.h>
#include <yaml.h>

bool yaml_emitter_initialize__fail = false;
bool yaml_emitter_open__fail = false;
bool yaml_emitter_dump__fail = false;
bool yaml_emitter_close__fail = false;

void reset_yaml_emitter__fail(void) {
	yaml_emitter_initialize__fail = false;
	yaml_emitter_open__fail = false;
	yaml_emitter_dump__fail = false;
	yaml_emitter_close__fail = false;
}

int __real_yaml_emitter_initialize(yaml_emitter_t *emitter);
int __wrap_yaml_emitter_initialize(yaml_emitter_t *emitter) {

	if (yaml_emitter_initialize__fail) {
		yaml_emitter_initialize__fail = false;
		return 0;
	}

	return __real_yaml_emitter_initialize(emitter);
}


int __real_yaml_emitter_open(yaml_emitter_t *emitter);
int __wrap_yaml_emitter_open(yaml_emitter_t *emitter) {

	if (yaml_emitter_open__fail) {
		yaml_emitter_open__fail = false;
		return 0;
	}

	return __real_yaml_emitter_open(emitter);
}


int __real_yaml_emitter_dump(yaml_emitter_t *emitter);
int __wrap_yaml_emitter_dump(yaml_emitter_t *emitter) {

	if (yaml_emitter_dump__fail) {
		yaml_emitter_dump__fail = false;
		return 0;
	}

	return __real_yaml_emitter_dump(emitter);
}


int __real_yaml_emitter_close(yaml_emitter_t *emitter);
int __wrap_yaml_emitter_close(yaml_emitter_t *emitter) {

	if (yaml_emitter_close__fail) {
		yaml_emitter_close__fail = false;
		return 0;
	}

	return __real_yaml_emitter_close(emitter);
}

