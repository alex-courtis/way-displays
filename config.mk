INCS = -Iinc -Ipro

CPPFLAGS += $(INCS) -D_GNU_SOURCE

ifdef VALGRIND
	OFLAGS = -O0 -g
else
	OFLAGS = -O3
endif
WFLAGS = -pedantic -Wall -Wextra -Werror -Wno-unused-parameter
COMPFLAGS = $(WFLAGS) $(OFLAGS)

CFLAGS += $(COMPFLAGS) -std=gnu17
CXXFLAGS += $(COMPFLAGS) -std=gnu++17

LDFLAGS +=

LDLIBS += -lwayland-client -lyaml-cpp -linput -ludev

CC = gcc
CXX = g++

