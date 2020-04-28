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

appname := tf2patcher
appfile := $(bindir)/$(appname)

ifeq ($(TARGET),64)
	restarget := pe-x86-64
	cctarget := -m64
else
	restarget := pe-i386
	cctarget := -m32
endif


ifeq ($(OS),Windows_NT)
	RM := del /Q /F
	MKDIR := mkdir
	slashfix = $(subst /,\,$(1)) # callable function for replacing slashes with backslashes

	appfile := $(addsuffix .exe,$(appfile))
	LFLAGS += -lpsapi # for mingw support

	resfilein := $(srcdir)\winres\$(appname).rc
	resfileout := $(objdir)\$(appname).res
else
	RM := rm -f
	MKDIR := mkdir -p
	slashfix = $(1)
endif



all: $(dirs) $(objects) winres
	$(CC) $(cctarget) $(CFLAGS) $(objects) $(resfileout) -o $(appfile) $(LFLAGS)

$(objects): $(objdir)/%.o: $(srcdir)/%.c $(headers)
	$(CC) $(cctarget) $(CFLAGS) -I$(srcdir) -c $< -o $@

$(dirs):
	$(MKDIR) $@


ifeq ($(OS),Windows_NT)
winres: $(resfilein)
	windres --input $(resfilein) --output $(resfileout) --output-format=coff --target=$(restarget)
endif

clean:
	$(RM) $(call slashfix,$(objects)) $(call slashfix,$(appfile)) $(call slashfix,$(resfileout))

.PHONY: all clean winres
