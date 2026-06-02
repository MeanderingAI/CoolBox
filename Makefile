# Top-level Makefile: OS dispatcher for CoolBox

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
include Makefile.osx
else ifeq ($(UNAME_S),Linux)
include Makefile.lin
else ifeq ($(OS),Windows_NT)
include Makefile.win
else
$(error Unsupported OS: $(UNAME_S). Please create a Makefile for your OS.)
endif
