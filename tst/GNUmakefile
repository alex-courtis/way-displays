include GNUmakefile

PKGS_TST = cmocka
CFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
CXXFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS_TST),$(shell pkg-config --libs $(p)))

OBJS =  tst/tst.o \
		$(patsubst %.c,%.o,$(wildcard tst/wrap-*.c)) \
		$(filter-out src/main.o,$(SRC_O)) \
		$(PRO_O) \
		$(LIB_O)

WRAPS = -Wl,$\
		--wrap=log_set_threshold,$\
		--wrap=log_,--wrap=log_error,--wrap=log_warn,--wrap=log_info,--wrap=log_error_nocap,$\
		--wrap=print_head,--wrap=print_mode,$\
		--wrap=wd_exit,--wrap=wd_exit_message

$(TST_O): $(TST_H) $(SRC_O) $(LIB_O) config.mk GNUmakefile tst/GNUmakefile

tst-cli: tst/tst-cli.o $(OBJS)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) $(WRAPS)

tst-head: tst/tst-head.o $(OBJS)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) $(WRAPS),--wrap=mode_dpi,--wrap=mode_user_mode,--wrap=mode_max_preferred

tst-layout: tst/tst-layout.o $(OBJS)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) $(WRAPS),--wrap=lid_is_closed,--wrap=head_find_mode,--wrap=head_auto_scale

tst-cfg: tst/tst-cfg.o $(OBJS)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) $(WRAPS)

tst-marshalling: tst/tst-marshalling.o $(OBJS)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) $(WRAPS)

tst-info: tst/tst-info.o $(OBJS)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) $(WRAPS)

tst-all: $(TST_E)
	@for e in $(^); do \
		echo ;\
		echo $$e ;\
		./$$e ;\
		if [ $$? -ne 0 ]; then \
		exit 1 ;\
		fi ;\
		done

