INCS += -Iinc -Ipro

CPPFLAGS += $(INCS) -D_POSIX_C_SOURCE=200809L

CFLAGS += -pedantic -O3 -std=c99 -Wall -Wextra -Wno-unused-parameter

LDFLAGS +=

LDLIBS += -lwayland-client

CC = gcc

