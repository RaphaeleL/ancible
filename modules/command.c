#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ancible.h"
#include "../include/core/context.h"
#include "../include/transport/runner.h"
#include "../include/modules/module.h"
#include "../include/modules/command.h"

/**
 * Execute the command module
 * 
 * @param context Execution context
 * @param args String containing module arguments
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int command_module_exec(context_t *context, const char *args, module_result_t *result) {
    if (!context || !result) {
        return ANCIBLE_ERROR;
    }
    
    // Initialize result
    module_result_init(result);
    
    // Extract command from args
    const char *cmd = args;
    
    if (!cmd) {
        result->failed = 1;
        result->msg = strdup("No command specified");
        return ANCIBLE_SUCCESS; // We return success because the module executed, but the task failed
    }
    
    // Execute command
    int ret = run_command(context, cmd, &result->cmd_result);
    if (ret != ANCIBLE_SUCCESS) {
        result->failed = 1;
        result->msg = strdup("Failed to execute command");
        return ANCIBLE_SUCCESS;
    }
    
    // Check command result
    if (result->cmd_result.exit_code != 0) {
        result->failed = 1;
        
        // Create message with exit code
        char msg[128];
        snprintf(msg, sizeof(msg), "Command failed with exit code %d", result->cmd_result.exit_code);
        result->msg = strdup(msg);
    } else {
        result->changed = 1;
        result->msg = strdup("Command executed successfully");
    }
    
    return ANCIBLE_SUCCESS;
}
