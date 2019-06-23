PROG := obscura

SOURCES := camera.c collidable.c main.c renderer.c scene.c world.c

OBJDIR := build
SRCDIR := src

CFLAGS	 ?= -std=gnu11 -Wall -Wextra -msse4.1
CPPFLAGS ?= -I$(SRCDIR) -D_GNU_SOURCE
LDFLAGS	 ?= -L$(SRCDIR) -rdynamic
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

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	@$(RM) -r $(OBJDIR)/*

.PHONY: distclean
distclean: clean
	@$(RM) $(PROG) core
