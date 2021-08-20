INCS += -Iinc -Ipro

CPPFLAGS += $(INCS) -D_POSIX_C_SOURCE=200809L

WFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS += -pedantic -O3 -std=c99 $(WFLAGS)

LDFLAGS +=

LDLIBS += -lwayland-client

CC = gcc

