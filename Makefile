# Install
BIN = hedge
BUILD_DIR = build
BIN_PATH = $(BUILD_DIR)/$(BIN)

# Flags
CFLAGS += -g -std=c89 -Wall -Wextra -pedantic
CPPFLAGS += -MMD -MP

SRC = main.c nuklear_impl.c
OBJ = $(SRC:%.c=$(BUILD_DIR)/%.o)
DEP = $(OBJ:.o=.d)

ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
LIBS = -lglfw3 -lopengl32 -lm -lGLEW32
else
	UNAME_S := $(shell uname -s)
	GLFW3 := $(shell pkg-config --libs glfw3)
	ifeq ($(UNAME_S),Darwin)
		LIBS := $(GLFW3) -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lm -lGLEW -L/usr/local/lib
	else
		LIBS = $(GLFW3) -lGL -lm -lGLEW
	endif
endif

all: $(BIN_PATH)

$(BIN_PATH): $(OBJ) | $(BUILD_DIR)
	$(CC) $(OBJ) $(CFLAGS) -o $@ $(LIBS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean

-include $(DEP)
