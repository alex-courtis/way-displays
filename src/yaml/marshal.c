#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml/marshal.h"

#include "log.h"

int write_handler(void *data, unsigned char *buffer, size_t size) {
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

static char *yaml_document_to_string(struct MC *c, const char *name) {
	char *yaml = NULL;

	yaml_emitter_t emitter;

	if (!yaml_emitter_initialize(&emitter)) {
		log_error("unable to marshal %s: yaml_emitter_initialize failed", name);
		return NULL;
	}

	yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);
	yaml_emitter_set_output(&emitter, write_handler, &yaml);

	if (!yaml_emitter_open(&emitter)) {
		log_error("unable to marshal %s: yaml_emitter_open failed", name);
		goto err;
	}

	if (!yaml_emitter_dump(&emitter, &c->d)) {
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

char *yaml_marshal(const void *data, yaml_doc_fn fn, const char *human) {
	if (!data)
		return NULL;

	char *yaml = NULL;

	struct MC c = { 0 };

	if (!yaml_document_initialize(&c.d, NULL, NULL, NULL, 1, 1)) {
		log_error("unable to marshal %s: yaml_document_initialize failed", human);
		return NULL;
	}

	if (!fn(&c, data))
		goto end;

	yaml = yaml_document_to_string(&c, human);

end:
	yaml_document_delete(&c.d);

	return yaml;
}

