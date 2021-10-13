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

$(SRC_O): $(INC_H) $(PRO_H)
$(PRO_O): $(PRO_H)

way-displays: $(SRC_O) $(PRO_O)
	$(CXX) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

$(PRO_H): $(PRO_X)
	wayland-scanner client-header $(@:.h=.xml) $@

$(PRO_C): $(PRO_X)
	wayland-scanner private-code $(@:.c=.xml) $@

clean:
	rm -f way-displays $(SRC_O) $(PRO_O) $(PRO_H) $(PRO_C) tags .copy

install: way-displays cfg.yaml
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f way-displays $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/way-displays
	mkdir -p $(DESTDIR)$(PREFIX_ETC)/etc/way-displays
	cp -f cfg.yaml $(DESTDIR)$(PREFIX_ETC)/etc/way-displays
	chmod 644 $(DESTDIR)$(PREFIX_ETC)/etc/way-displays/cfg.yaml

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/way-displays
	rm -rf $(DESTDIR)$(PREFIX_ETC)/etc/way-displays

# https://github.com/alex-courtis/arch/blob/7ca6c8d7f7aa910ec522470bb7a96ddb24c9a1ea/bin/ctags-something
tags: $(SRC_C) $(SRC_CXX) $(INC_H) $(PRO_H)
	which ctags-c > /dev/null 2>&1 && which ctags-c++ > /dev/null 2>&1 && \
		ctags-c   $(CFLAGS)   $(CPPFLAGS) --project-src $(SRC_C)   $(INC_H) $(PRO_H) && \
		ctags-c++ $(CXXFLAGS) $(CPPFLAGS) --project-src $(SRC_CXX) $(INC_H) $(PRO_H) --append || \
		true

.PHONY: all clean

