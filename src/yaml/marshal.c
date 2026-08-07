#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml/marshal.h"

#include "log.h"

struct MC mc = { 0 };

int yaml_write_handler(void *data, const unsigned char* const buffer, const size_t size) {
	if (!data)
		return 0;

	char **yaml = (char**)(data);

	if (*yaml) {
		char *current = *yaml;
		size_t len_current = strlen(current);

		*yaml = calloc(len_current + size + 1, sizeof(char));

		strncpy(*yaml, current, len_current);
		strncat(*yaml, (char*)buffer, size);

		free(current);
	} else {

		*yaml = calloc(1, size + 1);

		strncpy(*yaml, (char*)buffer, size);
	}

	return 1;
}

static char *yaml_doc_to_string(const char *name) {
	char *yaml = NULL;

	yaml_emitter_t emitter;

	if (!yaml_emitter_initialize(&emitter)) {
		log_error("unable to marshal %s: yaml_emitter_initialize failed", name);
		return NULL;
	}

	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);
	yaml_emitter_set_output(&emitter, (int(*)(void*, unsigned char*, size_t))yaml_write_handler, &yaml);

	if (!yaml_emitter_open(&emitter)) {
		log_error("unable to marshal %s: yaml_emitter_open failed", name);
		goto err;
	}

	if (!yaml_emitter_dump(&emitter, &mc.d)) {
		log_error("unable to marshal %s: yaml_emitter_dump failed", name);
		goto err;
	}

	if (!yaml_emitter_close(&emitter)) {
		log_warn("unable to marshal %s: yaml_emitter_close failed", name);
		goto err;
	}

	goto end;

err:
	if (yaml) {
		free(yaml);
		yaml = NULL;
	}

end:
	yaml_emitter_delete(&emitter);

	return yaml;
}

char *yaml_marshal(const void *data, fn_yaml_root_from_type fn, const char *human) {
	if (!data)
		return NULL;

	char *yaml = NULL;

	memset(&mc, 0, sizeof(struct MC));

	if (!yaml_document_initialize(&mc.d, NULL, NULL, NULL, 1, 1)) {
		log_error("unable to marshal %s: yaml_document_initialize failed", human);
		return NULL;
	}

	if (!fn(data))
		goto end;

	yaml = yaml_doc_to_string(human);

end:
	yaml_document_delete(&mc.d);

	memset(&mc, 0, sizeof(struct MC));

	return yaml;
}

