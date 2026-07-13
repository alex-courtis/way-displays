PKGS_TST = cmocka

CFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS_TST),$(shell pkg-config --libs $(p)))

INCS += -Ilib/alex-c/tst

CFLAGS += -Wno-unused-function

# remove libinput as it is 64-bit only and not used by tests
LDLIBS := $(filter-out -linput,$(LDLIBS))

$(TST_O): $(INC_H) $(TST_H) $(PRO_H) config.mk GNUmakefile tst/test.mk

# test executables exclude:
#   main
#   lid (completely mocked)
#   other tst-x.o
$(TST_E): $(filter-out src/main.o src/lid.o,$(SRC_O)) $(PRO_O) $(LIB_O) $(filter-out tst/tst%,$(TST_O))

# test-x builds tst/tst-x and executes it
test: $(patsubst tst/tst%,test%,$(TST_E))
test-%: tst/tst-%
	rm -f actual.* expected.* unexpected.*
	./$(^)

# test-x-vg builds tst/tst-x and executes it with valgrind
test-vg: $(patsubst tst/tst%,test%-vg,$(TST_E))
test-%-vg: tst/tst-%
	$(VALGRIND) ./$(^)

#
# common mocks
#

# log
LDFLAGS += -Wl,$\
		   --wrap=log_set_threshold,--wrap=log_get_threshold,$\
		   --wrap=log_,$\
		   --wrap=log_fatal,--wrap=log_fatal_errno,$\
		   --wrap=log_error,--wrap=log_error_errno,$\
		   --wrap=log_warn,$\
		   --wrap=log_info,$\
		   --wrap=log_debug

# process
LDFLAGS += -Wl,$\
		   --wrap=spawn_sh_cmd,$\
		   --wrap=wd_exit,--wrap=wd_exit_message

# lid
LDFLAGS += -Wl,$\
		   --wrap=g_lid_init,$\
		   --wrap=lid_free,--wrap=g_lid_destroy,$\
		   --wrap=g_lid_is_closed,--wrap=g_lid_update

# yaml
LDFLAGS += -Wl,$\
		   --wrap=yaml_document_initialize,$\
		   --wrap=yaml_parser_initialize,$\
		   --wrap=yaml_emitter_initialize,--wrap=yaml_emitter_open,--wrap=yaml_emitter_dump,--wrap=yaml_emitter_close

#
# test specific mocks
#
tst/tst-head: LDFLAGS += -Wl,$\
	--wrap=mode_dpi,$\
	--wrap=mode_best_satisfying,$\
	--wrap=mode_max_refresh,$\
	--wrap=callback

tst/tst-act: LDFLAGS += -Wl,$\
	--wrap=print_mode_fail,--wrap=print_adaptive_sync_fail,$\
	--wrap=callback,--wrap=callback_adaptive_sync_fail,--wrap=callback_mode_fail

tst/tst-desire: LDFLAGS += -Wl,$\
	--wrap=head_find_mode,$\
	--wrap=head_auto_scale

tst/tst-cfg-file: LDFLAGS += -Wl,$\
	--wrap=yaml_marshal,$\
	--wrap=fs_file_write,$\
	--wrap=fs_mkdir_p,$\
	--wrap=fs_canonical_path,$\
	--wrap=fd_wd_cfg_dir_destroy,$\
	--wrap=fd_wd_cfg_dir_create,$\
	--wrap=yaml_unmarshal_file

