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

tst-head: tst/tst-head.o
	$(CC) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

tst-layout: tst/tst-layout.o
	$(CC) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

test-all: $(TST_E)
	@for e in $(^); do echo; echo $$e; ./$$e; done

test-clean:
	rm -f $(TST_O) $(TST_E)

