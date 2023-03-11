include GNUmakefile

CPPFLAGS += -Itst
COMPFLAGS += -Wno-unused-function

PKGS_TST = cmocka
CFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
CXXFLAGS += $(foreach p,$(PKGS_TST),$(shell pkg-config --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS_TST),$(shell pkg-config --libs $(p)))

TST_H = $(wildcard tst/*.h)
TST_C = $(wildcard tst/*.c)
TST_CXX = $(wildcard tst/*.cpp)
TST_O = $(TST_C:.c=.o) $(TST_CXX:.cpp=.o)
TST_E = $(TST_O:tst/%.o=%)

$(TST_O): $(TST_H) $(SRC_O) config.mk GNUmakefile tst/GNUmakefile

tst-head: tst/tst-head.o $(filter-out src/main.o,$(SRC_O)) $(PRO_O)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) -Wl,--wrap=mode_dpi,--wrap=mode_user_mode,--wrap=log_info,--wrap=log_warn

tst-layout: tst/tst-layout.o $(filter-out src/main.o,$(SRC_O)) $(PRO_O)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

tst-cfg: tst/tst-cfg.o $(filter-out src/main.o,$(SRC_O)) $(PRO_O)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) -Wl,--wrap=log_warn

tst-marshalling: tst/tst-marshalling.o $(filter-out src/main.o,$(SRC_O)) $(PRO_O)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

tst-all: $(TST_E)
	@for e in $(^); do \
		echo ;\
		echo $$e ;\
		./$$e ;\
		if [ $$? -ne 0 ]; then \
			exit 1 ;\
		fi ;\
	done

tst-clean:
	rm -f $(TST_O) $(TST_E)

tst-iwyu: CC = $(IWYU) -Xiwyu --check_also="inc/*h"
tst-iwyu: CXX = $(IWYU) -Xiwyu --check_also="inc/marshalling.h"
tst-iwyu: clean $(TST_O)
IWYU = /usr/bin/include-what-you-use -Xiwyu --no_fwd_decls -Xiwyu --no_comments -Xiwyu --verbose=2

tst-cppcheck: $(SRC_C) $(SRC_CXX) $(INC_H) $(EXAMPLE_C) $(TST_H) $(TST_C) $(TST_CXX)
	cppcheck $(^) --enable=warning,unusedFunction,performance,portability $(CPPFLAGS)
