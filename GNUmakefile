include config.mk

INC_H = $(wildcard inc/*.h)

SRC_C = $(wildcard src/*.c)
SRC_CXX = $(wildcard src/*.cpp)
SRC_O = $(SRC_C:.c=.o) $(SRC_CXX:.cpp=.o)

EXAMPLE_C = $(wildcard examples/*.c)
EXAMPLE_O = $(EXAMPLE_C:.c=.o)

PRO_X = $(wildcard pro/*.xml)
PRO_H = $(PRO_X:.xml=.h)
PRO_C = $(PRO_X:.xml=.c)
PRO_O = $(PRO_X:.xml=.o)

TST_H = $(wildcard tst/*.h)
TST_C = $(wildcard tst/*.c)
TST_CXX = $(wildcard tst/*.cpp)
TST_O = $(TST_C:.c=.o) $(TST_CXX:.cpp=.o)
TST_E = $(patsubst tst/%.c,%,$(wildcard tst/tst-*.c))

all: way-displays

$(SRC_O): $(INC_H) $(PRO_H) config.mk GNUmakefile
$(PRO_O): $(PRO_H) config.mk GNUmakefile
$(EXAMPLE_O): $(INC_H) $(PRO_H) config.mk GNUmakefile

way-displays: $(SRC_O) $(PRO_O)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

example-client: $(EXAMPLE_O) $(filter-out src/main.o,$(SRC_O)) $(PRO_O)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

$(PRO_H): $(PRO_X)
	wayland-scanner client-header $(@:.h=.xml) $@

$(PRO_C): $(PRO_X)
	wayland-scanner private-code $(@:.c=.xml) $@

clean:
	rm -f way-displays example_client $(SRC_O) $(EXAMPLE_O) $(PRO_O) $(PRO_H) $(PRO_C) $(TST_O) $(TST_E)

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

# make -k iwyu
iwyu: CC = $(IWYU) -Xiwyu --check_also="inc/*h"
iwyu: CXX = $(IWYU) -Xiwyu --check_also="inc/marshalling.h"
iwyu: clean $(SRC_O) $(TST_O)
IWYU = /usr/bin/include-what-you-use -Xiwyu --no_fwd_decls -Xiwyu --no_comments -Xiwyu --verbose=2

cppcheck: $(SRC_C) $(SRC_CXX) $(INC_H) $(EXAMPLE_C) $(TST_H) $(TST_C)
	cppcheck $(^) --enable=warning,unusedFunction,performance,portability $(CPPFLAGS)

test:
	$(MAKE) -f tst/GNUmakefile tst-all

.PHONY: all clean install uninstall man cppcheck iwyu test clean-test tst-iwyu tst-cppcheck tst-all tst-clean

