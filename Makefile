PROG := obscura

SOURCES := camera.c collision.c geometry.c light.c main.c material.c renderer.c scene.c shade.c thread.c \
	visibility.c world.c

OBJDIR := build
SRCDIR := src

CFLAGS	 ?= -std=gnu11 -Wall -Wextra -msse -msse4.1
CPPFLAGS ?= -I$(SRCDIR) -D_GNU_SOURCE -DLEVEL1_DCACHE_LINESIZE=$(shell getconf LEVEL1_DCACHE_LINESIZE) \
	-DPAGESIZE=$(shell getconf PAGESIZE)
LDFLAGS	 ?= -L$(SRCDIR) -rdynamic -pthread
LDLIBS	 ?= -lm -lX11 -lXext -lyaml

ifeq ($(BUILD_DEBUG), 1)
	CFLAGS   += -g3 -ggdb -O0 -save-temps=obj -fverbose-asm -Wa,-adhlmn=$(OBJDIR)/main.lst
	CPPFLAGS += -DDEBUG
else
	CFLAGS   += -O3 -fno-math-errno
	CPPFLAGS += -DNDEBUG
endif

SRCS := $(patsubst %,$(SRCDIR)/%,$(SOURCES))
OBJS := $(patsubst %,$(OBJDIR)/%,$(SOURCES:c=o))

.PHONY: all
all: $(PROG)
ifeq ($(BUILD_DEBUG), 0)
	@strip -s $(PROG)
endif

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	@$(RM) -r $(OBJDIR)

.PHONY: distclean
distclean: clean
	@$(RM) $(PROG) core vgcore.*
