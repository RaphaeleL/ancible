# SPDX-License-Identifier: GPL-2.0

# Compiler and flags
CC = clang
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
TRANSPORT_SRC = $(wildcard $(TRANSPORT_DIR)/*.c)
TRANSPORT_OBJ = $(TRANSPORT_SRC:.c=.o)
MODULES_SRC = $(wildcard $(MODULES_DIR)/*.c)
MODULES_OBJ = $(MODULES_SRC:.c=.o)
CORE_SRC = $(wildcard $(CORE_DIR)/*.c)
CORE_OBJ = $(CORE_SRC:.c=.o)

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
TEST_CONDITION = $(TEST_DIR)/test_condition
TEST_BLOCKS = $(TEST_DIR)/test_blocks

# Beautify output
# ---------------------------------------------------------------------------
# Use 'make V=1' to see the full commands
# Use 'make -s' for silent mode
ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif
ifndef KBUILD_VERBOSE
  KBUILD_VERBOSE = 0
endif

quiet = quiet_
Q = @
ifneq ($(findstring 1, $(KBUILD_VERBOSE)),)
  quiet =
  Q =
endif
ifneq ($(findstring s,$(firstword -$(MAKEFLAGS))),)
  quiet = silent_
  override KBUILD_VERBOSE := 0
endif

# Command definitions for fancy printing
quiet_cmd_cc_o_c = CC      $<
      cmd_cc_o_c = $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
quiet_cmd_link = LD      $@
      cmd_link = $(CC) $(CFLAGS) $(INCLUDES) -o $@ $^
quiet_cmd_mkdir = MKDIR   $@
      cmd_mkdir = mkdir -p $@
quiet_cmd_clean = CLEAN   $<
      cmd_clean = rm -f $^

# Default target
.PHONY: all
all: prepare $(ANCIBLE_PLAYBOOK) $(TEST_CLI) $(TEST_ARGS) $(TEST_PARSER) $(TEST_INVENTORY) \
      $(TEST_CONTEXT) $(TEST_RUNNER) $(TEST_SSH) $(TEST_COMMAND) \
      $(TEST_COMMAND_MODULE) $(TEST_EXECUTOR) $(TEST_STATE) $(TEST_CONDITION) \
      $(TEST_BLOCKS)

# Prepare directories
.PHONY: prepare
prepare:
	$(Q)$(cmd_mkdir)
	$(Q)printf "%s\n" "$(quiet_cmd_mkdir)"

# Build the main executable
$(ANCIBLE_PLAYBOOK): $(CLI_OBJ) $(CORE_OBJ) $(TRANSPORT_OBJ) $(MODULES_OBJ)
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

# Compile source files
%.o: %.c
	$(Q)printf "%s\n" "$(quiet_cmd_cc_o_c)"
	$(Q)$(cmd_cc_o_c)

# Clean build artifacts
.PHONY: clean
clean:
	$(Q)printf "%s\n" "CLEAN   objects"
	$(Q)rm -f $(CLI_DIR)/*.o $(CORE_DIR)/*.o $(MODULES_DIR)/*.o $(TRANSPORT_DIR)/*.o
	$(Q)printf "%s\n" "CLEAN   executables"
	$(Q)rm -f $(ANCIBLE_PLAYBOOK) $(TEST_CLI) $(TEST_ARGS) $(TEST_PARSER) $(TEST_INVENTORY) \
	          $(TEST_CONTEXT) $(TEST_RUNNER) $(TEST_SSH) $(TEST_COMMAND) \
	          $(TEST_COMMAND_MODULE) $(TEST_EXECUTOR) $(TEST_STATE) \
	          $(TEST_CONDITION) $(TEST_BLOCKS)

# Run tests
.PHONY: test
test: $(ANCIBLE_PLAYBOOK) $(TEST_CLI) $(TEST_ARGS) $(TEST_PARSER) $(TEST_INVENTORY) \
      $(TEST_CONTEXT) $(TEST_RUNNER) $(TEST_SSH) $(TEST_COMMAND) \
      $(TEST_COMMAND_MODULE) $(TEST_EXECUTOR) $(TEST_STATE) $(TEST_CONDITION) \
      $(TEST_BLOCKS)
	@echo "Running unit tests..."
	$(Q)cd $(TEST_DIR) && ./test_cli
	$(Q)cd $(TEST_DIR) && ./test_args
	$(Q)cd $(TEST_DIR) && ./test_parser
	$(Q)cd $(TEST_DIR) && ./test_inventory
	$(Q)cd $(TEST_DIR) && ./test_context
	$(Q)cd $(TEST_DIR) && ./test_runner
	$(Q)cd $(TEST_DIR) && ./test_ssh
	$(Q)cd $(TEST_DIR) && ./test_command
	$(Q)cd $(TEST_DIR) && ./test_command_module
	$(Q)cd $(TEST_DIR) && ./test_executor
	$(Q)cd $(TEST_DIR) && ./test_state
	$(Q)cd $(TEST_DIR) && ./test_condition
	$(Q)cd $(TEST_DIR) && ./test_blocks

# Build test executables
$(TEST_CLI): $(TEST_DIR)/test_cli.c
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_ARGS): $(TEST_DIR)/test_args.c $(CLI_DIR)/args.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_PARSER): $(TEST_DIR)/test_parser.c $(CORE_DIR)/parser.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_INVENTORY): $(TEST_DIR)/test_inventory.c $(CORE_DIR)/inventory.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_CONTEXT): $(TEST_DIR)/test_context.c $(CORE_DIR)/context.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_RUNNER): $(TEST_DIR)/test_runner.c $(TRANSPORT_DIR)/runner.o $(TRANSPORT_DIR)/ssh.o $(CORE_DIR)/context.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_SSH): $(TEST_DIR)/test_ssh.c $(TRANSPORT_DIR)/ssh.o $(TRANSPORT_DIR)/runner.o $(CORE_DIR)/context.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_COMMAND): $(TEST_DIR)/test_command.c $(TRANSPORT_DIR)/runner.o $(TRANSPORT_DIR)/ssh.o $(CORE_DIR)/context.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_COMMAND_MODULE): $(TEST_DIR)/test_command_module.c $(MODULES_DIR)/command.o $(MODULES_DIR)/module.o $(TRANSPORT_DIR)/runner.o $(TRANSPORT_DIR)/ssh.o $(CORE_DIR)/context.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_EXECUTOR): $(TEST_DIR)/test_executor.c $(CORE_DIR)/executor.o $(CORE_DIR)/condition.o $(MODULES_DIR)/command.o $(MODULES_DIR)/module.o $(TRANSPORT_DIR)/runner.o $(TRANSPORT_DIR)/ssh.o $(CORE_DIR)/context.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_STATE): $(TEST_DIR)/test_state.c $(CORE_DIR)/state.o $(MODULES_DIR)/module.o $(TRANSPORT_DIR)/runner.o $(TRANSPORT_DIR)/ssh.o $(CORE_DIR)/context.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_CONDITION): $(TEST_DIR)/test_condition.c $(CORE_DIR)/condition.o $(CORE_DIR)/context.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)

$(TEST_BLOCKS): $(TEST_DIR)/test_blocks.c $(CORE_DIR)/parser.o $(CORE_DIR)/executor.o $(CORE_DIR)/condition.o $(MODULES_DIR)/module.o $(MODULES_DIR)/command.o $(TRANSPORT_DIR)/runner.o $(TRANSPORT_DIR)/ssh.o $(CORE_DIR)/context.o
	$(Q)printf "%s\n" "$(quiet_cmd_link)"
	$(Q)$(cmd_link)
