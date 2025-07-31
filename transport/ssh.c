#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../include/ancible.h"
#include "../include/transport/ssh.h"
#include "../include/transport/runner.h"

/**
 * Run a command remotely via SSH
 * 
 * @param context Execution context with host information
 * @param cmd Command to run
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int run_ssh(context_t *context, const char *cmd, command_result_t *result) {
    if (!context || !cmd || !result) {
        return ANCIBLE_ERROR;
    }
    
    // Get host and user from context
    const char *host = context_get_var(context, "ansible_host");
    if (!host) {
        host = context->host->name;
    }
    
    const char *user = context_get_var(context, "ansible_user");
    if (!user) {
        user = "root";  // Default user
    }
    
    // Build SSH command
    char ssh_cmd[4096];
    snprintf(ssh_cmd, sizeof(ssh_cmd), "ssh -o BatchMode=yes -o StrictHostKeyChecking=no %s@%s '%s'", 
             user, host, cmd);
    
    // Use run_local to execute the SSH command
    return run_local(ssh_cmd, result);
}
