include config.mk

INC_H = $(wildcard inc/*.h)

SRC_C = $(wildcard src/*.c)
SRC_CXX = $(wildcard src/*.cpp)
SRC_O = $(SRC_C:.c=.o) $(SRC_CXX:.cpp=.o)

PRO_X = $(wildcard pro/*.xml)
PRO_H = $(PRO_X:.xml=.h)
PRO_C = $(PRO_X:.xml=.c)
PRO_O = $(PRO_X:.xml=.o)

all: way-displays

$(SRC_O): $(INC_H) $(PRO_H) config.mk GNUmakefile
$(PRO_O): $(PRO_H) config.mk GNUmakefile

way-displays: $(SRC_O) $(PRO_O)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

$(PRO_H): $(PRO_X)
	wayland-scanner client-header $(@:.h=.xml) $@

$(PRO_C): $(PRO_X)
	wayland-scanner private-code $(@:.c=.xml) $@

clean:
	rm -f way-displays $(SRC_O) $(PRO_O) $(PRO_H) $(PRO_C) tags .copy

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

cppcheck: $(SRC_C) $(SRC_CXX) $(INC_H)
	cppcheck $(^) --enable=warning,unusedFunction,performance,portability $(CPPFLAGS)

.PHONY: all clean install uninstall man cppcheck

