CC := clang

TARGET := gameboy

SRC_DIR := src
INC_DIR := include
BUILD_DIR := build

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS := $(shell sdl2-config --libs)

CFLAGS := -std=c23 -Wall -Wextra -Wpedantic -Wshadow \
          -I$(INC_DIR) $(SDL_CFLAGS)

DEBUG_FLAGS := -O0 -g3 -fsanitize=address,undefined \
               -fno-omit-frame-pointer

RELEASE_FLAGS := -O2 -DNDEBUG

LDFLAGS := $(SDL_LIBS)

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all debug release clean run rebuild

all: debug

debug: CFLAGS += $(DEBUG_FLAGS)
debug: LDFLAGS += -fsanitize=address,undefined
debug: $(TARGET)

release: CFLAGS += $(RELEASE_FLAGS)
release: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

run: debug
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

rebuild: clean all

-include $(DEPS)