CC := gcc

TARGET := gameboy

SRC_DIR := src
INC_DIR := include
BUILD_DIR := build

CFLAGS := -std=c23 -Wall -Wextra -Wpedantic -Wshadow \
          -I$(INC_DIR)

DEBUG_FLAGS := -O0 -g3
RELEASE_FLAGS := -O2 -DNDEBUG

LDFLAGS :=

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all debug release clean run rebuild

all: debug

debug: CFLAGS += $(DEBUG_FLAGS)
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

rebuild: clean debug

-include $(DEPS)