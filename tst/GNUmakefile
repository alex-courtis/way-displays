include GNUmakefile

CPPFLAGS += -Itst
COMPFLAGS += -Wno-unused-function

PKGS_TST = cmocka
CFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
CXXFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS_TST),$(shell pkg-config --libs $(p)))

OBJS = $(patsubst %.c,%.o,$(wildcard tst/wrap-*.c)) \
	   $(filter-out src/main.o,$(SRC_O)) \
	   $(PRO_O)

WRAPS = -Wl,--wrap=log_error,--wrap=log_warn,--wrap=log_info,--wrap=log_debug,--wrap=log_error_nocap,--wrap=log_,--wrap=wd_exit

$(TST_O): $(TST_H) $(SRC_O) config.mk GNUmakefile tst/GNUmakefile

tst-cli: tst/tst-cli.o $(OBJS)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) $(WRAPS)

tst-head: tst/tst-head.o $(OBJS)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) $(WRAPS),--wrap=mode_dpi,--wrap=mode_user_mode

tst-layout: tst/tst-layout.o $(OBJS)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) $(WRAPS)

tst-cfg: tst/tst-cfg.o $(OBJS)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) $(WRAPS)

tst-marshalling: tst/tst-marshalling.o $(OBJS)
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

tst-iwyu: CC = $(IWYU) -Xiwyu --check_also="inc/*h"
tst-iwyu: CXX = $(IWYU) -Xiwyu --check_also="inc/marshalling.h"
tst-iwyu: clean $(TST_O)
IWYU = /usr/bin/include-what-you-use -Xiwyu --no_fwd_decls -Xiwyu --no_comments -Xiwyu --verbose=2

tst-cppcheck: $(SRC_C) $(SRC_CXX) $(INC_H) $(EXAMPLE_C) $(TST_H) $(TST_C) $(TST_CXX)
	cppcheck $(^) --enable=warning,unusedFunction,performance,portability $(CPPFLAGS)
