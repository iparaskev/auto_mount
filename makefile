SHELL := /bin/bash

# VARIABLES
CC = gcc
RM = rm -f
MKDIR = mkdir -p
CFLAGS = -I
DIRS = bin
DEPS = constants.h
RUN = auto_mount

# DIRECTORIES
SRC = src
BIN = bin
HEADERS = headers
OBJD = obj


# COMMANDS
all: $(DIRS) $(RUN)

$(DIRS):
	$(MKDIR) $@

auto_mount: $(SRC)/auto_mount.c $(HEADERS)/$(DEPS)
	$(CC) -o $(BIN)/$@ $< $(CFLAGS) $(HEADERS)

clean_all:
	$(RM) $(BIN)/*

