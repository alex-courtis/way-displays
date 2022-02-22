VERSION ?= "1.4.0"

PREFIX ?= /usr/local
PREFIX_ETC ?= /usr/local

INCS = -Iinc -Ipro

CPPFLAGS += $(INCS) -D_GNU_SOURCE -DVERSION=\"$(VERSION)\"

OFLAGS = -O3
WFLAGS = -pedantic -Wall -Wextra -Werror -Wno-unused-parameter
COMPFLAGS = $(WFLAGS) $(OFLAGS)

CFLAGS += $(COMPFLAGS) -std=gnu17
CXXFLAGS += $(COMPFLAGS) -std=gnu++17

LDFLAGS +=

LDLIBS += -lwayland-client -lyaml-cpp -linput -ludev -lstdc++

CC = gcc
CXX = g++

