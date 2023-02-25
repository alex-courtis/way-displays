include GNUmakefile

CPPFLAGS += -Itst

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
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) -Wl,--wrap=mode_dpi,--wrap=log_info

tst-layout: tst/tst-layout.o
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

tst-all: $(TST_E)
	@for e in $(^); do echo; echo $$e; ./$$e; done

tst-clean:
	rm -f $(TST_O) $(TST_E)

tst-iwyu: CC = $(IWYU) -Xiwyu --check_also="inc/*h"
tst-iwyu: CXX = $(IWYU) -Xiwyu --check_also="inc/marshalling.h"
tst-iwyu: clean $(TST_O)
IWYU = /usr/bin/include-what-you-use -Xiwyu --no_fwd_decls -Xiwyu --no_comments -Xiwyu --verbose=2
