#include <cmocka.h>
#include <yaml.h>

bool yaml_document_initialize__fail = false;
bool yaml_document_add_mapping__fail = false;

bool yaml_emitter_initialize__fail = false;
bool yaml_emitter_open__fail = false;
bool yaml_emitter_dump__fail = false;
bool yaml_emitter_close__fail = false;

bool yaml_parser_initialize__fail = false;
bool yaml_parser_load__fail = false;

void reset_yaml_fails(void) {
	yaml_document_initialize__fail = false;
	yaml_document_add_mapping__fail = false;

	yaml_emitter_initialize__fail = false;
	yaml_emitter_open__fail = false;
	yaml_emitter_dump__fail = false;
	yaml_emitter_close__fail = false;

	yaml_parser_initialize__fail = false;
	yaml_parser_load__fail = false;
}


int __real_yaml_document_initialize(yaml_document_t *document, yaml_version_directive_t *version_directive, yaml_tag_directive_t *tag_directives_start, yaml_tag_directive_t *tag_directives_end, int start_implicit, int end_implicit);
int __wrap_yaml_document_initialize(yaml_document_t *document, yaml_version_directive_t *version_directive, yaml_tag_directive_t *tag_directives_start, yaml_tag_directive_t *tag_directives_end, int start_implicit, int end_implicit) {

	if (yaml_document_initialize__fail) {
		yaml_document_initialize__fail = false;
		return 0;
	}

	return __real_yaml_document_initialize(document, version_directive, tag_directives_start, tag_directives_end, start_implicit, end_implicit);
}


int __real_yaml_document_add_mapping(yaml_document_t *document, const yaml_char_t *tag, yaml_mapping_style_t style);
int __wrap_yaml_document_add_mapping(yaml_document_t *document, const yaml_char_t *tag, yaml_mapping_style_t style) {

	if (yaml_document_add_mapping__fail) {
		yaml_document_add_mapping__fail = false;
		return 0;
	}

	return __real_yaml_document_add_mapping(document, tag, style);
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


int __real_yaml_emitter_dump(yaml_emitter_t *emitter, yaml_document_t *document);
int __wrap_yaml_emitter_dump(yaml_emitter_t *emitter, yaml_document_t *document) {

	if (yaml_emitter_dump__fail) {
		yaml_emitter_dump__fail = false;
		return 0;
	}

	return __real_yaml_emitter_dump(emitter, document);
}


int __real_yaml_emitter_close(yaml_emitter_t *emitter);
int __wrap_yaml_emitter_close(yaml_emitter_t *emitter) {

	if (yaml_emitter_close__fail) {
		yaml_emitter_close__fail = false;
		return 0;
	}

	return __real_yaml_emitter_close(emitter);
}



int __real_yaml_parser_initialize(yaml_parser_t *parser);
int __wrap_yaml_parser_initialize(yaml_parser_t *parser) {

	if (yaml_parser_initialize__fail) {
		yaml_parser_initialize__fail = false;
		return 0;
	}

	return __real_yaml_parser_initialize(parser);
}


int __real_yaml_parser_load(yaml_parser_t *parser, yaml_document_t *document);
int __wrap_yaml_parser_load(yaml_parser_t *parser, yaml_document_t *document) {

	if (yaml_parser_load__fail) {
		yaml_parser_load__fail = false;
		return 0;
	}

	return __real_yaml_parser_load(parser, document);
}
