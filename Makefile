# Default compiler settings
WARNINGS = -Wall
LD_LIBS := -lpthread
INCLUDE := include

# use local includes, but other values from calling Makefile
ifeq "$(origin INCLUDES)" "undefined"
	CFLAGS += -I$(INCLUDE)
endif
ifeq "$(origin CC)" "undefined"
	CC = gcc
endif
ifeq "$(origin AR)" "undefined"
	AR = ar
endif

CFLAGS  += $(WARNINGS) -g
LDFLAGS += $(LD_LIBS) -g

COBJS	:= $(patsubst %.c, %.o, $(wildcard *.c))
OBJS	:= $(COBJS)

# target
TARGET = timer
LIBRARY = lib$(TARGET).a
LIBRARY_DIR = .
LIBRARY_TARGET = ${LIBRARY_DIR}/${LIBRARY}

all: $(OBJS)
	$(AR) cr $(LIBRARY_TARGET) $^

clean:
	rm -rf $(OBJS) $(LIBRARY_TARGET)
