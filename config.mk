VERSION ?= "1.14.2-SNAPSHOT"

PREFIX ?= /usr/local
PREFIX_ETC ?= /usr/local
ROOT_ETC ?= /etc

INCS = -Iinc -Ipro -Ilib/col/inc

CPPFLAGS += $(INCS) -D_GNU_SOURCE -DVERSION=\"$(VERSION)\" -DROOT_ETC=\"$(ROOT_ETC)\"

OFLAGS = -O3
WFLAGS = -pedantic \
		 -Wall \
		 -Wextra \
		 -Werror \
		 -Wimplicit-fallthrough \
		 -Wno-unused-parameter \
		 -Wno-format-zero-length
DFLAGS = -g
MFLAGS = 
COMPFLAGS = $(WFLAGS) $(OFLAGS) $(DFLAGS) $(MFLAGS)

CFLAGS += $(COMPFLAGS) \
		  -std=gnu17 \
		  -Wold-style-definition \
		  -Wstrict-prototypes

CXXFLAGS += $(COMPFLAGS) \
			-std=gnu++17 \
			-Wno-c99-extensions

LDFLAGS += $(MFLAGS)

ifeq (,$(filter-out DragonFly FreeBSD NetBSD OpenBSD,$(shell uname -s)))
	PKGS += epoll-shim libinotify
endif

PKGS += wayland-client yaml-cpp libinput libudev
PKG_CONFIG ?= pkg-config
CFLAGS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --cflags $(p)))
CXXFLAGS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --cflags $(p)))
LDLIBS += $(foreach p,$(PKGS),$(shell $(PKG_CONFIG) --libs $(p)))

ifneq (,$(findstring -m32,$(MFLAGS)))
	VG_SUPP = --suppressions=bld/vg.cmocka.32.supp
endif

CC = gcc
CXX = g++
