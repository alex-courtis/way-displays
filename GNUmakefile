include config.mk

INC_H = $(wildcard inc/*.h)

SRC_C = $(wildcard src/*.c)
SRC_CXX = $(wildcard src/*.cpp)
SRC_O = $(SRC_C:.c=.o) $(SRC_CXX:.cpp=.o)

LIB_H = $(wildcard lib/alex-c-collections/inc/*.h)
LIB_C = $(wildcard lib/alex-c-collections/src/*.c)
LIB_O = $(LIB_C:.c=.o)

EXAMPLE_C = $(wildcard examples/*.c)
EXAMPLE_O = $(EXAMPLE_C:.c=.o)

PRO_X = $(wildcard pro/*.xml)
PRO_H = $(PRO_X:.xml=.h)
PRO_C = $(PRO_X:.xml=.c)
PRO_O = $(PRO_X:.xml=.o)

TST_H = $(wildcard tst/*.h)
TST_C = $(wildcard tst/*.c)
TST_O = $(TST_C:.c=.o)
TST_E = $(patsubst tst/%.c,%,$(wildcard tst/tst-*.c))
TST_T = $(patsubst tst%,test%,$(TST_E))

all: way-displays

$(SRC_O): $(INC_H) $(PRO_H) config.mk GNUmakefile
$(LIB_O): $(LIB_H) $(LIB_C) config.mk GNUmakefile
$(PRO_O): $(PRO_H) config.mk GNUmakefile
$(EXAMPLE_O): $(INC_H) $(PRO_H) config.mk GNUmakefile

way-displays: $(SRC_O) $(PRO_O) $(LIB_O)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

example-client: $(EXAMPLE_O) $(filter-out src/main.o,$(SRC_O)) $(PRO_O) $(LIB_O)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

$(PRO_H): $(PRO_X)
	wayland-scanner client-header $(@:.h=.xml) $@

$(PRO_C): $(PRO_X)
	wayland-scanner private-code $(@:.c=.xml) $@

clean:
	rm -f way-displays example_client $(SRC_O) $(EXAMPLE_O) $(PRO_O) $(PRO_H) $(PRO_C) $(LIB_O) $(TST_O) $(TST_E)

install: way-displays way-displays.1 cfg.yaml
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f way-displays $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/way-displays
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	cp -f way-displays.1 $(DESTDIR)$(PREFIX)/share/man/man1
	chmod 644 $(DESTDIR)$(PREFIX)/share/man/man1/way-displays.1
	mkdir -p $(DESTDIR)$(PREFIX_ETC)/etc/way-displays
	cp -f cfg.yaml $(DESTDIR)$(PREFIX_ETC)/etc/way-displays
	chmod 644 $(DESTDIR)$(PREFIX_ETC)/etc/way-displays/cfg.yaml

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/way-displays
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/way-displays.1
	rm -rf $(DESTDIR)$(PREFIX_ETC)/etc/way-displays

man: way-displays.1.pandoc
	sed -i -e "3i % `date +%Y/%m/%d`" -e "3d" $(^)
	pandoc -s --wrap=none -f markdown -t man $(^) -o $(^:.pandoc=)

iwyu: override CC = $(IWYU) -Xiwyu --check_also="inc/*h"
iwyu: override CXX = $(IWYU) -Xiwyu --check_also="inc/marshalling.h"
iwyu: clean $(SRC_O) $(TST_O)
IWYU = include-what-you-use -Xiwyu --no_fwd_decls -Xiwyu --error=1 -Xiwyu --verbose=3

cppcheck: $(SRC_C) $(SRC_CXX) $(INC_H) $(EXAMPLE_C) $(TST_H) $(TST_C)
	cppcheck $(^) --enable=warning,unusedFunction,performance,portability --suppressions-list=.cppcheck.supp --error-exitcode=1 $(CPPFLAGS)

%-vg: VALGRIND = valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --gen-suppressions=all --suppressions=.vg.supp
%-vg: % ;

test: $(TST_T)
test-vg: $(TST_T)

$(TST_T): EXE = $(patsubst test%,tst%,$(@))
$(TST_T):
	$(MAKE) -f tst/GNUmakefile $(EXE)
	$(VALGRIND) ./$(EXE)

.PHONY: all clean install uninstall man cppcheck iwyu test test-vg $(TST_T)

.NOTPARALLEL: iwyu test test-vg
