INCS += -Iinc -Ipro

CPPFLAGS += $(INCS) -D_GNU_SOURCE

WFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter
COMPFLAGS = -pedantic -O3 $(WFLAGS)

CFLAGS += $(COMPFLAGS) -std=gnu17
CXXFLAGS += $(COMPFLAGS) -std=gnu++17

LDFLAGS +=

LDLIBS += -lwayland-client -lyaml-cpp -linput -ludev

CC = gcc
CXX = g++

