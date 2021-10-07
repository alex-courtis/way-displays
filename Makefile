include config.mk

INC_H = $(wildcard inc/*.h)

SRC_C = $(wildcard src/*.c)
SRC_CXX = $(wildcard src/*.cpp)
SRC_O = $(SRC_C:.c=.o) $(SRC_CXX:.cpp=.o)

PRO_X = $(wildcard pro/*.xml)
PRO_H = $(PRO_X:.xml=.h)
PRO_C = $(PRO_X:.xml=.c)
PRO_O = $(PRO_X:.xml=.o)

all: way-displays tags .copy

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
	mkdir -p $(DESTDIR)$(PREFIX)/etc/way-displays
	cp -f cfg.yaml $(DESTDIR)$(PREFIX)/etc/way-displays
	chmod 644 $(DESTDIR)$(PREFIX)/etc/way-displays/cfg.yaml

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/way-displays
	rm -rf $(DESTDIR)$(PREFIX)/etc/way-displays

# https://github.com/alex-courtis/arch/blob/b530f331dacaaba27484593a87ca20a9f53ab73f/home/bin/ctags-something
tags: $(SRC_C) $(INC_H) $(PRO_H)
	which ctags-c > /dev/null 2>&1 && \
		ctags-c $(CFLAGS) $(CPPFLAGS) --project-src $(^) || \
		true

.copy: way-displays
	scp $(^) alw@gigantor:/home/alw/bin
	@touch .copy

.PHONY: all clean

