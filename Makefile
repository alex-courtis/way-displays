include config.mk

INC_H = $(wildcard inc/*.h)

SRC_C = $(wildcard src/*.c)
SRC_O = $(SRC_C:.c=.o)

PRO_X = $(wildcard pro/*.xml)
PRO_H = $(PRO_X:.xml=.h)
PRO_C = $(PRO_X:.xml=.c)
PRO_O = $(PRO_X:.xml=.o)

TST_C = $(wildcard tst/*.c)
TST_O = tst/test.o

TST_LDLIBS  = -lcmocka
TST_WRAPS = -Wl,--wrap=first,--wrap=second

all: way-layout-displays test tags .copy

$(SRC_O): $(INC_H) $(PRO_H)
$(PRO_O): $(PRO_H)
$(TST_O): $(INC_H) $(PRO_H) $(TST_C)

way-layout-displays: $(SRC_O) $(PRO_O)
	$(CC) -o $(@) $(^) $(LDFLAGS) $(LDLIBS)

$(PRO_H): $(PRO_X)
	wayland-scanner client-header $(@:.h=.xml) $@

$(PRO_C): $(PRO_X)
	wayland-scanner private-code $(@:.c=.xml) $@

test: $(filter-out %main.o,$(SRC_O)) $(PRO_O) $(TST_O)
	$(CC) -o $(@) $(^) $(LDFLAGS) $(LDLIBS) $(TST_LDLIBS) $(TST_WRAPS)
	./test

clean:
	rm -f way-layout-displays test $(SRC_O) $(PRO_O) $(PRO_H) $(PRO_C) $(TST_O) tags .copy

tags: $(SRC_C) $(INC_H) $(PRO_H) $(TST_C)
	ctags --fields=+S --c-kinds=+p \
		$(^) \
		/usr/include/wayland*.h \
		/usr/include/cmocka*.h \
		/usr/include/std*h

.copy: way-layout-displays
	scp $(^) alw@gigantor:/home/alw/bin || true
	scp $(^) emperor:/home/alex/bin || true
	@touch .copy

.PHONY: all clean

