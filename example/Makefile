# Default compiler settings
WARNINGS = -Wall
LD_LIBS := -lpthread
INCLUDE := .

# use local includes, but other values from calling Makefile
ifeq "$(origin INCLUDES)" "undefined"
	CFLAGS += -I../
endif
ifeq "$(origin CC)" "undefined"
	CC = gcc
endif
ifeq "$(origin AR)" "undefined"
	AR = ar
endif
ifeq "$(origin STRIP)" "undefined"
	STRIP = strip
endif

LIBRARY_DIR = ../
TIMERLIB = timer

CFLAGS  += $(WARNINGS) -O2 -g
CFLAGS += -I$(INCLUDE)
LDFLAGS += $(LD_LIBS)
LDFLAGS += -L$(LIBRARY_DIR) -l$(TIMERLIB)

COBJS	:= $(patsubst %.c, %.o, $(wildcard *.c))
OBJS	:= $(COBJS)

# target
EXAMPLE1 = example1
EXAMPLE2 = example2

all: example1 example2
.PHONY : all example1 example2 clean

example1: $(OBJS)
	$(CC) $(EXAMPLE1).o -o $(EXAMPLE1) $(LDFLAGS)
	$(STRIP) $(EXAMPLE1)
	size $(EXAMPLE1)

example2: $(OBJS)
	$(CC) $(EXAMPLE2).o -o $(EXAMPLE2) $(LDFLAGS)
	$(STRIP) $(EXAMPLE2)
	size $(EXAMPLE2)

clean:
	rm -rf $(OBJS) $(EXAMPLE1) $(EXAMPLE2)
