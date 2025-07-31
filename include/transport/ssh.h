#ifndef ANCIBLE_SSH_H
#define ANCIBLE_SSH_H

#include "../core/context.h"
#include "runner.h"

/**
 * Run a command remotely via SSH
 * 
 * @param context Execution context with host information
 * @param cmd Command to run
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int run_ssh(context_t *context, const char *cmd, command_result_t *result);

#endif /* ANCIBLE_SSH_H */
