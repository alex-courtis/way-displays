INCS += -Iinc -Ipro

CPPFLAGS += $(INCS) -D_GNU_SOURCE

WFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS += -pedantic -O3 -std=gnu17 $(WFLAGS)

LDFLAGS +=

LDLIBS += -lwayland-client

CC = gcc

