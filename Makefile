# Makefile for samplecontrol tools and library

CC = gcc
AR = ar
LD = gcc
CFLAGS =  -I./include -D__DEBUG__=1 -D__ASSERT_LEVEL__=4

LDFLAGS = -L/opt/homebrew/Cellar/glfw/3.4/lib/

ROOTDIR = ./

CP = cp
ECHO = echo

BUILD_DIR = ./build

# Desktop uses raylib for its screen and console devices
RAYLIB_DIR = ../sc_screendevice/raylib
RAYLIB_LIB_PATH = $(RAYLIB_DIR)/build/raylib
RAYLIB_INCLUDE_PATH = $(RAYLIB_DIR)/build/raylib/include
RAY_LINKCMDS =  -lraylib -framework CoreVideo -framework OpenGL -framework IOKit \
				-framework Cocoa -framework Carbon  -lm -lpthread -ldl -lglfw3

SDL_CFLAGS = -I/opt/homebrew/include/ -D_THREAD_SAFE
SDL_LDFLAGS = -L/opt/homebrew/lib -lSDL2 -lSDL2_ttf -lSDL2_image \
              -lfreetype -lpng -lwebp -ltiff -ljpeg -lbz2 -lz

CFLAGS += -I$(RAYLIB_INCLUDE_PATH) -D__DESKTOP__

CFLAGS += $(SDL_CFLAGS)

LDFLAGS += -L$(RAYLIB_LIB_PATH) $(RAY_LINKCMDS)

LDFLAGS += $(SDL_LDFLAGS)

SCASM_SOURCES = 	src/scasm.c \
					src/sc_asmdis.c \
					src/util.c

SCDIS_SOURCES =		src/scdis.c \
					src/sc_asmdis.c \
					src/util.c

SCEM_SOURCES =		src/scem.c \
					src/console.c \
					src/screen.c \
					src/file.c \
					src/util.c \
					src/SDL_FontCache.c \
					src/lfqueue.c

SCASM_HEADERS = 	include/util.h
SCEM_HEADERS  = 	include/util.h \
					include/lfqueue.h


SCASM = scasm
SCDIS = scdis
SCEM  = scem

SCASM_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SCASM_SOURCES:.c=.o)))
SCDIS_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SCDIS_SOURCES:.c=.o)))
SCEM_OBJECTS  = $(addprefix $(BUILD_DIR)/,$(notdir $(SCEM_SOURCES:.c=.o)))

DEPS = $(addprefix $(BUILD_DIR)/,$(notdir $(SCASM_SOURCES:.c=.d)))
DEPS += $(addprefix $(BUILD_DIR)/,$(notdir $(SCDIS_SOURCES:.c=.d)))
DEPS += $(addprefix $(BUILD_DIR)/,$(notdir $(SCEM_SOURCES:.c=.d)))

.PHONY: all
all:: $(BUILD_DIR)/$(SCASM) $(BUILD_DIR)/$(SCDIS) $(BUILD_DIR)/$(SCEM)

$(DEPS):

-include $(DEPS)

vpath %.c $(sort $(dir $(SCASM_SOURCES)))
vpath %.c $(sort $(dir $(SCDIS_SOURCES)))
vpath %.c $(sort $(dir $(SCEM_SOURCES)))
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

$(BUILD_DIR)/$(SCEM): $(SCEM_OBJECTS) Makefile
	$(ECHO) linking $<
	$(CC) $(LDFLAGS) -o $@ $(SCEM_OBJECTS)
	$(ECHO) successs

#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)/$(SCASM) $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d

.PHONY: clean all