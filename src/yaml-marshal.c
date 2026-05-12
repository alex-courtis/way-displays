#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "yaml-marshal.h"

#include "log.h"

struct MarshalCtx marshal_ctx = { 0 };

static int write_handler(void *data, unsigned char *buffer, size_t size) {
	if (!data)
		return 0;

	char **yaml = (char**)(data);

	*yaml = calloc(1, size + 1);

	strncpy(*yaml, (char*)buffer, size);

	return 1;
}

static char *yaml_document_to_string(yaml_document_t *document, const char *name) {
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

	if (!yaml_emitter_dump(&emitter, document)) {
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

char *struct_to_yaml(const void *data, struct_to_yaml_fn fn, const char *name) {
	if (!data) {
		return NULL;
	}

	char *yaml = NULL;

	yaml_document_t document;
	marshal_ctx.doc = &document;

	if (!yaml_document_initialize(&document, NULL, NULL, NULL, 1, 1)) {
		log_error("unable to marshal %s: yaml_document_initialize failed", name);
		return NULL;
	}

	if (!fn(data))
		goto end;

	yaml = yaml_document_to_string(&document, name);

end:
	yaml_document_delete(&document);

	marshal_ctx.doc = NULL;

	return yaml;
}

