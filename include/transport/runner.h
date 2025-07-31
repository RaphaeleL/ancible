#ifndef ANCIBLE_RUNNER_H
#define ANCIBLE_RUNNER_H

#include "../core/context.h"

/**
 * Structure to hold command result
 */
typedef struct {
    int exit_code;       // Exit code of the command
    char *stdout_data;   // Standard output of the command
    char *stderr_data;   // Standard error of the command
} command_result_t;

/**
 * Run a command locally
 * 
 * @param cmd Command to run
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int run_local(const char *cmd, command_result_t *result);

/**
 * Run a command on a host (local or remote)
 * 
 * @param context Execution context with host information
 * @param cmd Command to run
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int run_command(context_t *context, const char *cmd, command_result_t *result);

/**
 * Free resources used by a command result
 * 
 * @param result Pointer to result structure to free
 */
void command_result_free(command_result_t *result);

/**
 * Print command result (for debugging)
 * 
 * @param result Pointer to result structure to print
 */
void command_result_print(const command_result_t *result);

#endif /* ANCIBLE_RUNNER_H */
