# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic
INCLUDES = -I./include

# Directories
SRC_DIR = .
BIN_DIR = $(SRC_DIR)/bin
CLI_DIR = $(SRC_DIR)/cli
CORE_DIR = $(SRC_DIR)/core
MODULES_DIR = $(SRC_DIR)/modules
TRANSPORT_DIR = $(SRC_DIR)/transport
TEST_DIR = $(SRC_DIR)/tests/unit

# Main executable
ANCIBLE_PLAYBOOK = $(BIN_DIR)/ancible-playbook

# Source files
CLI_SRC = $(wildcard $(CLI_DIR)/*.c)
CLI_OBJ = $(CLI_SRC:.c=.o)

# Transport source files
TRANSPORT_SRC = $(wildcard $(TRANSPORT_DIR)/*.c)
TRANSPORT_OBJ = $(TRANSPORT_SRC:.c=.o)

# Module source files
MODULES_SRC = $(wildcard $(MODULES_DIR)/*.c)
MODULES_OBJ = $(MODULES_SRC:.c=.o)

# Default target
all: prepare $(ANCIBLE_PLAYBOOK)

# Prepare directories
prepare:
	@mkdir -p $(BIN_DIR)

# Core source files
CORE_SRC = $(wildcard $(CORE_DIR)/*.c)
CORE_OBJ = $(CORE_SRC:.c=.o)

# Build the main executable
$(ANCIBLE_PLAYBOOK): $(CLI_OBJ) $(CORE_OBJ) $(TRANSPORT_OBJ) $(MODULES_OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(CLI_DIR)/*.o $(CORE_DIR)/*.o $(MODULES_DIR)/*.o $(TRANSPORT_DIR)/*.o
	rm -f $(ANCIBLE_PLAYBOOK)
	rm -f $(TEST_CLI) $(TEST_ARGS) $(TEST_PARSER) $(TEST_INVENTORY) $(TEST_CONTEXT) $(TEST_RUNNER) $(TEST_SSH) $(TEST_COMMAND) $(TEST_COMMAND_MODULE) $(TEST_EXECUTOR) $(TEST_STATE)

# Test executables
TEST_CLI = $(TEST_DIR)/test_cli
TEST_ARGS = $(TEST_DIR)/test_args
TEST_PARSER = $(TEST_DIR)/test_parser
TEST_INVENTORY = $(TEST_DIR)/test_inventory
TEST_CONTEXT = $(TEST_DIR)/test_context
TEST_RUNNER = $(TEST_DIR)/test_runner
TEST_SSH = $(TEST_DIR)/test_ssh
TEST_COMMAND = $(TEST_DIR)/test_command
TEST_COMMAND_MODULE = $(TEST_DIR)/test_command_module
TEST_EXECUTOR = $(TEST_DIR)/test_executor
TEST_STATE = $(TEST_DIR)/test_state

# Run tests
test: $(ANCIBLE_PLAYBOOK) $(TEST_CLI) $(TEST_ARGS) $(TEST_PARSER) $(TEST_INVENTORY) $(TEST_CONTEXT) $(TEST_RUNNER) $(TEST_SSH) $(TEST_COMMAND) $(TEST_COMMAND_MODULE) $(TEST_EXECUTOR) $(TEST_STATE)
	@echo "Running unit tests..."
	@cd $(TEST_DIR) && ./test_cli
	@cd $(TEST_DIR) && ./test_args
	@cd $(TEST_DIR) && ./test_parser
	@cd $(TEST_DIR) && ./test_inventory
	@cd $(TEST_DIR) && ./test_context
	@cd $(TEST_DIR) && ./test_runner
	@cd $(TEST_DIR) && ./test_ssh
	@cd $(TEST_DIR) && ./test_command
	@cd $(TEST_DIR) && ./test_command_module
	@cd $(TEST_DIR) && ./test_executor
	@cd $(TEST_DIR) && ./test_state

# Build test_cli
$(TEST_CLI): $(TEST_DIR)/test_cli.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Build test_args
$(TEST_ARGS): $(TEST_DIR)/test_args.c $(CLI_DIR)/args.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Build test_parser
$(TEST_PARSER): $(TEST_DIR)/test_parser.c $(CORE_DIR)/parser.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Build test_inventory
$(TEST_INVENTORY): $(TEST_DIR)/test_inventory.c $(CORE_DIR)/inventory.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Build test_context
$(TEST_CONTEXT): $(TEST_DIR)/test_context.c $(CORE_DIR)/context.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Build test_runner
$(TEST_RUNNER): $(TEST_DIR)/test_runner.c $(TRANSPORT_DIR)/runner.o $(TRANSPORT_DIR)/ssh.o $(CORE_DIR)/context.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Build test_ssh
$(TEST_SSH): $(TEST_DIR)/test_ssh.c $(TRANSPORT_DIR)/ssh.o $(TRANSPORT_DIR)/runner.o $(CORE_DIR)/context.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Build test_command
$(TEST_COMMAND): $(TEST_DIR)/test_command.c $(TRANSPORT_DIR)/runner.o $(TRANSPORT_DIR)/ssh.o $(CORE_DIR)/context.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Build test_command_module
$(TEST_COMMAND_MODULE): $(TEST_DIR)/test_command_module.c $(MODULES_DIR)/command.o $(MODULES_DIR)/module.o $(TRANSPORT_DIR)/runner.o $(TRANSPORT_DIR)/ssh.o $(CORE_DIR)/context.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Build test_executor
$(TEST_EXECUTOR): $(TEST_DIR)/test_executor.c $(CORE_DIR)/executor.o $(MODULES_DIR)/command.o $(MODULES_DIR)/module.o $(TRANSPORT_DIR)/runner.o $(TRANSPORT_DIR)/ssh.o $(CORE_DIR)/context.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Build test_state
$(TEST_STATE): $(TEST_DIR)/test_state.c $(CORE_DIR)/state.o $(MODULES_DIR)/module.o $(TRANSPORT_DIR)/runner.o $(TRANSPORT_DIR)/ssh.o $(CORE_DIR)/context.o
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

.PHONY: all prepare clean test