include GNUmakefile

PKGS_TST = cmocka
CFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
CXXFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS_TST),$(shell pkg-config --libs $(p)))

OBJS =  tst/util.o \
		$(patsubst %.c,%.o,$(wildcard tst/wrap-*.c)) \
		$(filter-out src/main.o,$(SRC_O)) \
		$(PRO_O)

WRAPS_COMMON = -Wl,$\
	--wrap=log_set_threshold,$\
	--wrap=log_,--wrap=log_error,--wrap=log_warn,--wrap=log_info,--wrap=log_debug,--wrap=log_error_errno,$\
	--wrap=spawn_sh_cmd,--wrap=wd_exit,--wrap=wd_exit_message

tst-head: WRAPS=,$\
	--wrap=mode_dpi,$\
	--wrap=mode_user_mode,$\
	--wrap=mode_max_preferred

tst-layout: WRAPS=,$\
	--wrap=lid_is_closed,$\
	--wrap=head_find_mode,$\
	--wrap=print_head,--wrap=print_mode,--wrap=report_adaptive_sync_fail,$\
	--wrap=head_auto_scale

tst-cfg-file: WRAPS=,$\
	--wrap=marshal_cfg,$\
	--wrap=file_write,$\
	--wrap=mkdir_p,$\
	--wrap=fd_wd_cfg_dir_destroy,$\
	--wrap=fd_wd_cfg_dir_create


$(TST_O): $(TST_H) $(SRC_O) config.mk GNUmakefile tst/GNUmakefile

$(TST_E): $(TST_O) $(OBJS)
	$(CXX) -o $(@) tst/$(@).o $(OBJS) $(LDFLAGS) $(LDLIBS) $(WRAPS_COMMON)$(WRAPS)
