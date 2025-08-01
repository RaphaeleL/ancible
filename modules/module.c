#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ancible.h"
#include "../include/modules/module.h"

/**
 * Initialize module result structure
 * 
 * @param result Pointer to result structure to initialize
 */
void module_result_init(module_result_t *result) {
    if (!result) {
        return;
    }
    
    memset(result, 0, sizeof(module_result_t));
}

/**
 * Free resources used by a module result
 * 
 * @param result Pointer to result structure to free
 */
void module_result_free(module_result_t *result) {
    if (!result) {
        return;
    }
    
    free(result->msg);
    result->msg = NULL;
    
    command_result_free(&result->cmd_result);
}

/**
 * Print module result (for debugging)
 * 
 * @param result Pointer to result structure to print
 */
void module_result_print(const module_result_t *result) {
    if (!result) {
        return;
    }
    
    printf("Module Result:\n");
    printf("  Changed: %s\n", result->changed ? "yes" : "no");
    printf("  Failed: %s\n", result->failed ? "yes" : "no");
    printf("  Skipped: %s\n", result->skipped ? "yes" : "no");
    printf("  Message: %s\n", result->msg ? result->msg : "(none)");
    
    if (result->cmd_result.stdout_data || result->cmd_result.stderr_data) {
        printf("  Command Result:\n");
        printf("    Exit Code: %d\n", result->cmd_result.exit_code);
        if (result->cmd_result.stdout_data && strlen(result->cmd_result.stdout_data) > 0) {
            printf("    Stdout: %s\n", result->cmd_result.stdout_data);
        }
        if (result->cmd_result.stderr_data && strlen(result->cmd_result.stderr_data) > 0) {
            printf("    Stderr: %s\n", result->cmd_result.stderr_data);
        }
    }
}
