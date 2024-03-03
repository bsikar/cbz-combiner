# Compiler and Linker definitions
COMPILER = gcc
LINKER = gcc
NAME = cbz-combiner

# Compiler flags
DEBUG_CFLAGS = -std=c17 -g -Wall   -Wunused-function
RELEASE_CFLAGS = -std=c17 -O2 -Wall   -Wunused-function
CFLAGS = $(DEBUG_CFLAGS)

# Linker flags
LFLAGS = -lzip -lpng -ljpeg -lhpdf -lm

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source and object files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPENDS = $(OBJECTS:.o=.d)

# Default target
.PHONY: all
all: $(BIN_DIR)/$(NAME)

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling $<..."
	@$(COMPILER) $(CFLAGS) -MMD -c $< -o $@

# Link
$(BIN_DIR)/$(NAME): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	@echo "Linking $(NAME)..."
	@$(LINKER) $(OBJECTS) $(LFLAGS) -o $@
	@echo "-----"
	@echo "$(NAME) built successfully!"

# Include dependencies
-include $(DEPENDS)

# Clean
.PHONY: clean
clean:
	@echo "Cleaning up..."
	@rm -f $(OBJECTS) $(DEPENDS) $(BIN_DIR)/$(NAME)
	@rmdir $(OBJ_DIR) $(BIN_DIR) 2>/dev/null || true

# Run
# must do `make run ARGS="--help"`
.PHONY: run
run:
	@$(MAKE) CFLAGS="$(RELEASE_CFLAGS)"
	@echo "Running $(NAME) with arguments: $(ARGS)"
	@./$(BIN_DIR)/$(NAME) $(ARGS)

# Additional Rules
.PHONY: gdb debug valgrind release
debug:
	@$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"

gdb:
	@$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"
	@echo "Starting GDB..."
	@gdb ./$(BIN_DIR)/$(NAME)

# `make valgrind ARGS="--help" VAL_ARGS="--leak-check=yes --leak-check=full --tool=callgrind"`
# `make valgrind VAL_ARGS="--leak-check=full --show-leak-kinds=all -s" ARGS="--help"`
valgrind:
	@$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"
	@valgrind $(VAL_ARGS) ./$(BIN_DIR)/$(NAME) $(ARGS)

release:
	@$(MAKE) CFLAGS="$(RELEASE_CFLAGS)"

