VERSION ?= "1.12.1-SNAPSHOT"

PREFIX ?= /usr/local
PREFIX_ETC ?= /usr/local
ROOT_ETC ?= /etc

INCS = -Iinc -Ipro -Ilib/col/inc

CPPFLAGS += $(INCS) -D_GNU_SOURCE -DVERSION=\"$(VERSION)\" -DROOT_ETC=\"$(ROOT_ETC)\"

OFLAGS = -O3
WFLAGS = -pedantic -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-result
DFLAGS = -g
COMPFLAGS = $(WFLAGS) $(OFLAGS) $(DFLAGS)

CFLAGS += $(COMPFLAGS) -std=gnu17 -Wold-style-definition -Wstrict-prototypes
CXXFLAGS += $(COMPFLAGS) -std=gnu++17

LDFLAGS +=

ifeq (,$(filter-out DragonFly FreeBSD NetBSD OpenBSD,$(shell uname -s)))
PKGS += epoll-shim libinotify
endif

PKGS += wayland-client yaml-cpp libinput libudev
PKG_CONFIG ?= pkg-config
CFLAGS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --cflags $(p)))
CXXFLAGS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --libs $(p)))

CC = gcc
CXX = g++
