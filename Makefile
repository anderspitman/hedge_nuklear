# Install
BIN = hedge
BIN_PATH = bin/$(BIN)

# Flags
CFLAGS += -g -std=c89 -Wall -Wextra -pedantic
CPPFLAGS += -MMD -MP

SRC = main.c
OBJ = $(SRC:.c=.o)
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

$(BIN_PATH): $(OBJ) | bin
	$(CC) $(OBJ) $(CFLAGS) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

bin:
	@mkdir -p $@

clean:
	rm -f $(OBJ) $(DEP) $(BIN_PATH)

.PHONY: all clean

-include $(DEP)
