# Makefile for samplecontrol tools and library

CC = gcc
AR = ar
LD = gcc
CFLAGS =  -I./include -D__DEBUG__=1 -D__ASSERT_LEVEL__=4

LDFLAGS = 

ROOTDIR = ./

CP = cp
ECHO = echo

BUILD_DIR = ./build

SCASM_SOURCES = 	src/scasm.c \
					src/lexer.c \
					src/util.c

SCASM_HEADERS = 	include/lexer.h \
					include/lexer.h

SCASM = scasm

SCASM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SCASM_SOURCES:.c=.o)))

DEPS = $(addprefix $(BUILD_DIR)/,$(notdir $(SCASM_SOURCES:.c=.d)))

.PHONY: all
all:: $(BUILD_DIR)/$(SCASM)

$(DEPS):

-include $(DEPS)

vpath %.c $(sort $(dir $(SCASM_SOURCES)))
vpath %.c src

$(BUILD_DIR)/%.d: %.c
	@set -e; rm -f $@; \
	gcc $(CFLAGS) -MM $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(ECHO) $(DEPS)
	$(ECHO) compiling $<
	$(CC) -c $(CFLAGS) $< -o $@ -MMD -MP

$(BUILD_DIR)/$(SCASM): $(SCASM_OBJECTS) Makefile
	$(ECHO) linking $<
	$(CC) $(LDFLAGS) -o $@ $(SCASM_OBJECTS)
	$(ECHO) successs

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)/$(SCASM) $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d

.PHONY: clean all