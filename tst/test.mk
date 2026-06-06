PKGS_TST = cmocka

CFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS_TST),$(shell pkg-config --libs $(p)))

CFLAGS += -Wno-unused-function
INCS += -Ilib/alex-c/tst

# filter libinput as it is 64-bit only and not used by tests
LDLIBS := $(filter-out -linput,$(LDLIBS))

# filter lid.o as it links the 64-bit only libinput and is completely mocked
TESTED_O = $(filter-out src/main.o src/lid.o,$(SRC_O)) $(PRO_O)

$(TST_O): $(INC_H) $(TST_H) $(UTIL_H) config.mk GNUmakefile tst/test.mk

# don't include other tst/tst-x
$(TST_E): $(TESTED_O) $(filter-out tst/tst%,$(TST_O))

#
# link test-x targets to tst/tst-x and execute them
#
test: $(patsubst tst/tst%,test%,$(TST_E))
test-%: tst/tst-%
	./$(^)

test-vg: $(patsubst tst/tst%,test%-vg,$(TST_E))
test-%-vg: tst/tst-%
	$(VALGRIND) ./$(^)

#
# common wraps
#
LDFLAGS += -Wl,$\
		   --wrap=log_set_threshold,--wrap=log_get_threshold,$\
		   --wrap=log_,--wrap=log_fatal,--wrap=log_error,--wrap=log_warn,--wrap=log_info,--wrap=log_debug,--wrap=log_error_errno,--wrap=log_error_errno,--wrap=log_fatal_errno,$\
		   --wrap=spawn_sh_cmd,--wrap=wd_exit,--wrap=wd_exit_message,$\
		   --wrap=lid_init,--wrap=lid_is_closed,--wrap=lid_update,--wrap=lid_free,--wrap=lid_destroy,$\
		   --wrap=yaml_document_initialize,--wrap=yaml_emitter_initialize,--wrap=yaml_emitter_open,--wrap=yaml_emitter_dump,--wrap=yaml_emitter_close,--wrap=yaml_parser_initialize

#
# test specific wraps
#
tst/tst-head: LDFLAGS += -Wl,$\
	--wrap=mode_dpi,$\
	--wrap=mode_user_mode,$\
	--wrap=mode_max_preferred,$\
	--wrap=call_back

tst/tst-layout: LDFLAGS += -Wl,$\
	--wrap=head_find_mode,$\
	--wrap=print_mode_fail,--wrap=print_adaptive_sync_fail,$\
	--wrap=call_back,--wrap=call_back_adaptive_sync_fail,--wrap=call_back_mode_fail,$\
	--wrap=head_auto_scale

tst/tst-cfg-file: LDFLAGS += -Wl,$\
	--wrap=yaml_marshal,$\
	--wrap=file_write,$\
	--wrap=mkdir_p,$\
	--wrap=fd_wd_cfg_dir_destroy,$\
	--wrap=fd_wd_cfg_dir_create

tst/tst-server: LDFLAGS += -Wl,$\
	--wrap=cfg_resolve_file_path,$\
	--wrap=yaml_unmarshal_file

