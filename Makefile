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


# Desktop uses raylib for its screen and console devices
RAYLIB_DIR = ../sc_screendevice/raylib
RAYLIB_LIB_PATH = $(RAYLIB_DIR)/build/raylib
RAYLIB_INCLUDE_PATH = $(RAYLIB_DIR)/build/raylib/include
RAY_LINKCMDS =  -framework CoreVideo -framework OpenGL -framework IOKit \
				-framework Cocoa -framework Carbon  -lm -lpthread -ldl -lglfw3

CFLAGS += -I$(RAYLIB_INCLUDE_PATH) -D__DESKTOP__

SCASM_SOURCES = 	src/scasm.c \
					src/sc_asmdis.c \
					src/util.c

SCDIS_SOURCES =		src/scdis.c \
					src/sc_asmdis.c \
					src/util.c

SCASM_HEADERS = 	include/util.h 

SCASM = scasm
SCDIS = scdis

SCASM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SCASM_SOURCES:.c=.o)))
SCDIS_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SCDIS_SOURCES:.c=.o)))

DEPS = $(addprefix $(BUILD_DIR)/,$(notdir $(SCASM_SOURCES:.c=.d)))
DEPS += $(addprefix $(BUILD_DIR)/,$(notdir $(SCDIS_SOURCES:.c=.d)))

.PHONY: all
all:: $(BUILD_DIR)/$(SCASM) $(BUILD_DIR)/$(SCDIS)

$(DEPS):

-include $(DEPS)

vpath %.c $(sort $(dir $(SCASM_SOURCES)))
vpath %.c $(sort $(dir $(SCDIS_SOURCES)))
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

$(BUILD_DIR)/$(SCDIS): $(SCDIS_OBJECTS) Makefile
	$(ECHO) linking $<
	$(CC) $(LDFLAGS) -o $@ $(SCDIS_OBJECTS)
	$(ECHO) successs

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)/$(SCASM) $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d

.PHONY: clean all