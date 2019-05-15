# Default compiler settings
WARNINGS = -Wall
LD_LIBS := -lpthread
INCLUDE := $(shell pwd)

CROSS ?= #arm-linux-gnueabihf-
CC = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip

CFLAGS  += $(WARNINGS) -O2 -g
CFLAGS += -I$(INCLUDE)
LDFLAGS += $(LD_LIBS)

COBJS	:= $(patsubst %.c, %.o, $(wildcard *.c))
OBJS	:= $(COBJS)

EXAMPLES_DIR := $(shell pwd)/example
# target
TARGET = timer
LIBRARY = lib$(TARGET).a
LIBRARY_DIR = .
LIBRARY_TARGET = ${LIBRARY_DIR}/${LIBRARY}

all: timer examples
.PHONY : all timer examples clean

export CC AR STRIP INCLUDE CFLAGS LDFLAGS

timer: $(OBJS)
	$(AR) cr $(LIBRARY_TARGET) $^

examples: timer
	$(MAKE) -s -C $(EXAMPLES_DIR) all

clean:
	$(MAKE) -s -C $(EXAMPLES_DIR) clean
	rm -rf $(OBJS) $(LIBRARY_TARGET)
