# GNU makefile for tf2patcher

CC = gcc
CFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter -O2 -std=c11

bindir := bin
objdir := obj
srcdir := src
dirs := $(bindir) $(objdir) $(srcdir)

headers := $(wildcard $(srcdir)/*.h)
sources := $(wildcard $(srcdir)/*.c)
objects := $(sources:$(srcdir)/%.c=$(objdir)/%.o)

appname := $(bindir)/tf2patcher


ifeq ($(OS),Windows_NT)
	RM := del /Q /F
	MKDIR := mkdir
	appname := $(addsuffix .exe,$(appname))
	slashfix = $(subst /,\,$(1)) # callable function for replacing slashes with backslashes
	LFLAGS += -lpsapi # for mingw support
else
	RM := rm -f
	MKDIR := mkdir -p
	slashfix = $(1)
endif


all: $(dirs) $(objects)
	$(CC) $(ARCH) $(CFLAGS) $(objects) -o $(appname) $(LFLAGS)

$(objects): $(objdir)/%.o: $(srcdir)/%.c $(headers)
	$(CC) $(ARCH) $(CFLAGS) -I$(srcdir) -c $< -o $@

$(dirs):
	$(MKDIR) $@

win32: ARCH=-m32
win32: all

win64: ARCH=-m64
win64: all

clean:
	$(RM) $(call slashfix,$(objects)) $(call slashfix,$(appname))

.PHONY: all clean win32 win64
