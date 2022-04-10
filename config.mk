VERSION ?= "1.5.2"

PREFIX ?= /usr/local
PREFIX_ETC ?= /usr/local

INCS = -Iinc -Ipro

CPPFLAGS += $(INCS) -D_GNU_SOURCE -DVERSION=\"$(VERSION)\"

OFLAGS = -O3
WFLAGS = -pedantic -Wall -Wextra -Werror -Wno-unused-parameter
COMPFLAGS = $(WFLAGS) $(OFLAGS)

CFLAGS += $(COMPFLAGS) -std=gnu17 -Wold-style-definition -Wstrict-prototypes
CXXFLAGS += $(COMPFLAGS) -std=gnu++17

LDFLAGS +=

ifeq (,$(filter-out DragonFly FreeBSD NetBSD OpenBSD,$(shell uname -s)))
PKGS += epoll-shim libinotify
endif

PKGS += wayland-client yaml-cpp libinput libudev
CFLAGS += $(foreach p,$(PKGS),$(shell pkg-config --cflags $(p)))
CXXFLAGS += $(foreach p,$(PKGS),$(shell pkg-config --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS),$(shell pkg-config --libs $(p)))

CC = gcc
CXX = g++

