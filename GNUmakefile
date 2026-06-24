include config.mk

INC_H = $(wildcard inc/*.h) $(wildcard inc/*/*.h) $(wildcard lib/alex-c/inc/*.h)

SRC_C = $(wildcard src/*.c) $(wildcard src/*/*.c)
SRC_O = $(SRC_C:.c=.o)

LIB_C = $(wildcard lib/*/src/*.c)
LIB_O = $(LIB_C:.c=.o)

EXAMPLE_C = $(wildcard examples/*.c)
EXAMPLE_O = $(EXAMPLE_C:.c=.o)
EXAMPLE_E = $(EXAMPLE_C:.c=)

PRO_X = $(wildcard pro/*.xml)
PRO_H = $(PRO_X:.xml=.h)
PRO_C = $(PRO_X:.xml=.c)
PRO_O = $(PRO_X:.xml=.o)

TST_H = $(wildcard tst/*.h) $(wildcard lib/alex-c/tst/*.h)
TST_C = $(wildcard tst/*.c) $(wildcard lib/alex-c/tst/*.c)
TST_O = $(TST_C:.c=.o)
TST_E = $(filter tst/tst%,$(TST_O:.o=))

all: way-displays

clean:
	rm -f way-displays $(SRC_O) $(PRO_O) $(LIB_O) $(PRO_H) $(PRO_C) $(TST_O) $(TST_E) $(EXAMPLE_E) $(EXAMPLE_O) actual.* expected.*
	find . -name '*.gcno' -type f -delete -print
	find . -name '*.gcda' -type f -delete -print

$(SRC_O): CFLAGS += $(COVCFLAGS)
$(SRC_O): $(INC_H) $(PRO_H) config.mk GNUmakefile
$(LIB_O): config.mk GNUmakefile
$(PRO_O): $(PRO_H) config.mk GNUmakefile
$(EXAMPLE_O): $(INC_H) $(PRO_H) config.mk GNUmakefile

#
# executable
#
way-displays: $(SRC_O) $(PRO_O) $(LIB_O)
	$(CC) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)
	@test -x ../deploy.sh && ../deploy.sh || true

#
# protocols
#
$(PRO_H): $(PRO_X)
	wayland-scanner client-header $(@:.h=.xml) $@

$(PRO_C): $(PRO_X)
	wayland-scanner private-code $(@:.c=.xml) $@

#
# deploy
#
install: way-displays doc/way-displays.1 examples/cfg.yaml
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f way-displays $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/way-displays
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man1
	cp -f doc/way-displays.1 $(DESTDIR)$(PREFIX)/share/man/man1
	chmod 644 $(DESTDIR)$(PREFIX)/share/man/man1/way-displays.1
	mkdir -p $(DESTDIR)$(PREFIX_ETC)/etc/way-displays
	cp -f examples/cfg.yaml $(DESTDIR)$(PREFIX_ETC)/etc/way-displays
	chmod 644 $(DESTDIR)$(PREFIX_ETC)/etc/way-displays/cfg.yaml

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/way-displays
	rm -f $(DESTDIR)$(PREFIX)/share/man/man1/way-displays.1
	rm -rf $(DESTDIR)$(PREFIX_ETC)/etc/way-displays

#
# doc
#
man: doc/way-displays.1.pandoc
	sed -i -e "3i % `date +%Y/%m/%d`" -e "3d" $(^)
	pandoc -s --wrap=none -f markdown -t man $(^) -o $(^:.pandoc= )

#
# iwyu
#
iwyu: override CC = include-what-you-use \
	-Xiwyu --no_fwd_decls \
	-Xiwyu --error=1 \
	-Xiwyu --verbose=3 \
	-Xiwyu --mapping_file=.iwyu.imp \
	-Xiwyu --check_also="inc/*h" \
	-Xiwyu --check_also="lib/alex-c/inc/*h" \
	-Xiwyu --check_also="tst/*h" \
	-Xiwyu --check_also="lib/alex-c/tst/*h"
iwyu: INCS += -Ilib/alex-c/tst
iwyu: clean $(SRC_O) $(LIB_O) $(TST_O) $(EXAMPLE_O)

#
# cppcheck
#
cppcheck: INCS += -Ilib/alex-c/tst
cppcheck: $(SRC_C) $(LIB_C) $(INC_H) $(EXAMPLE_C) $(TST_H) $(TST_C)
	cppcheck $(^) \
		--enable=all \
		--inconclusive \
		--check-level=exhaustive \
		--inline-suppr \
		--suppressions-list=bld/cppcheck.supp \
		--error-exitcode=1 \
		$(CPPFLAGS)

# TODO --inconclusive

#
# tests only included when executing test targets
#
ifneq (,$(or $(findstring test,$(MAKECMDGOALS)), $(findstring tst/tst,$(MAKECMDGOALS))))

include tst/test.mk

endif

#
# examples
#
examples: $(EXAMPLE_E)
examples/%: examples/%.o $(filter-out src/main.o,$(SRC_O)) $(PRO_O) $(LIB_O)
	$(CC) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

.PHONY: all clean install uninstall man cppcheck iwyu

.NOTPARALLEL: iwyu test test-vg
