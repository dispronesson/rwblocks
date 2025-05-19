CC = gcc
CFLAGS_DEBUG = -g -ggdb -std=c17 -pedantic -W -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable
CFLAGS_RELEASE = -std=c17 -pedantic -W -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable

SRC_DIR = src
BUILD_DIR = build

DEBUG_DIR = $(BUILD_DIR)/debug
RELEASE_DIR = $(BUILD_DIR)/release

SRC_FILES = $(SRC_DIR)/main.c $(SRC_DIR)/func.c

OBJ_FILES_DEBUG = $(patsubst $(SRC_DIR)/%.c, $(DEBUG_DIR)/%.o, $(SRC_FILES))
OBJ_FILES_RELEASE = $(patsubst $(SRC_DIR)/%.c, $(RELEASE_DIR)/%.o, $(SRC_FILES))

EXEC_NAME = prog

all: debug

debug: $(DEBUG_DIR)/$(EXEC_NAME)

release: $(RELEASE_DIR)/$(EXEC_NAME)

define compile_rule
$2/%.o: $1/%.c | $2/
	$(CC) $3 -c $$< -o $$@
endef

$(eval $(call compile_rule,$(SRC_DIR),$(DEBUG_DIR),$(CFLAGS_DEBUG)))
$(eval $(call compile_rule,$(SRC_DIR),$(RELEASE_DIR),$(CFLAGS_RELEASE)))

define link_rule
$2/$3: $1
	$(CC) $$^ -o $$@
endef

$(eval $(call link_rule,$(OBJ_FILES_DEBUG),$(DEBUG_DIR),$(EXEC_NAME)))
$(eval $(call link_rule,$(OBJ_FILES_RELEASE),$(RELEASE_DIR),$(EXEC_NAME)))

%/:
	mkdir -p $@

clean:
	rm -fr $(BUILD_DIR)

.PHONY: all debug release clean