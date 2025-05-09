include GNUmakefile

PKGS_TST = cmocka
CFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
CXXFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS_TST),$(shell pkg-config --libs $(p)))

# filter libinput as it is 64-bit only
LDLIBS := $(filter-out -linput,$(LDLIBS))

# filter lid.o as it links the 64-bit only libinput
OBJS =  tst/util.o \
		$(patsubst %.c,%.o,$(wildcard tst/wrap-*.c)) \
		$(filter-out src/main.o src/lid.o,$(SRC_O)) \
		$(PRO_O)

# 
COMPFLAGS += -Wno-int-to-pointer-cast

WRAPS_COMMON = -Wl,$\
	--wrap=log_set_threshold,--wrap=log_get_threshold,$\
	--wrap=log_,--wrap=log_fatal,--wrap=log_error,--wrap=log_warn,--wrap=log_info,--wrap=log_debug,--wrap=log_error_errno,--wrap=log_error_errno,--wrap=log_fatal_errno,$\
	--wrap=spawn_sh_cmd,--wrap=wd_exit,--wrap=wd_exit_message,$\
	--wrap=lid_init,--wrap=lid_is_closed,--wrap=lid_update,--wrap=lid_free,--wrap=lid_destroy

tst-head: WRAPS=,$\
	--wrap=mode_dpi,$\
	--wrap=mode_user_mode,$\
	--wrap=mode_max_preferred,$\
	--wrap=call_back

tst-layout: WRAPS=,$\
	--wrap=head_find_mode,$\
	--wrap=print_mode_fail,--wrap=print_adaptive_sync_fail,$\
	--wrap=call_back,--wrap=call_back_adaptive_sync_fail,--wrap=call_back_mode_fail,$\
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
