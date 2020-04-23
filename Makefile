# GNU makefile for tf2patcher

CC = gcc
CFLAGS += -Wall -Wextra -Werror -O2 -std=c11

bindir := bin
objdir := obj
srcdir := src

headers := $(wildcard $(srcdir)/*.h)
sources := $(wildcard $(srcdir)/*.c)
objects := $(sources:$(srcdir)/%.c=$(objdir)/%.o)

appname := $(bindir)/tf2patcher


ifeq ($(OS),Windows_NT)
	RM := del /Q /F
	appname := $(addsuffix .exe,$(appname))
	slashfix = $(subst /,\,$(1)) # callable function for replacing slashes with backslashes
	LFLAGS += -lpsapi # for mingw support
else
	RM := rm -f
	slashfix = $(1)
endif


all: $(objects)
	$(CC) $(CFLAGS) $(objects) -o $(appname) $(LFLAGS)

$(objects): $(objdir)/%.o: $(srcdir)/%.c
	$(CC) $(CFLAGS) -I$(srcdir) -c $< -o $@

$(srcdir)/%.c: headers

clean:
	$(RM) $(call slashfix,$(objects)) $(call slashfix,$(appname))

.PHONY: all clean
