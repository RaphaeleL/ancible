#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ancible.h"
#include "../include/core/executor.h"
#include "../include/modules/command.h"

#define MAX_MODULES 32

// Module registry
static module_registry_entry_t registry[MAX_MODULES];
static int registry_count = 0;

/**
 * Initialize the module registry
 * 
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int executor_init(void) {
    // Clear registry
    memset(registry, 0, sizeof(registry));
    registry_count = 0;
    
    // Register built-in modules
    if (executor_register_module("command", command_module_exec) != ANCIBLE_SUCCESS) {
        fprintf(stderr, "Error: Failed to register command module\n");
        return ANCIBLE_ERROR;
    }
    
    return ANCIBLE_SUCCESS;
}

/**
 * Register a module
 * 
 * @param name Module name
 * @param func Module function
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int executor_register_module(const char *name, module_func_t func) {
    if (!name || !func) {
        return ANCIBLE_ERROR;
    }
    
    // Check if module already exists
    for (int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i].name, name) == 0) {
            fprintf(stderr, "Error: Module '%s' already registered\n", name);
            return ANCIBLE_ERROR;
        }
    }
    
    // Check if registry is full
    if (registry_count >= MAX_MODULES) {
        fprintf(stderr, "Error: Module registry is full (max %d modules)\n", MAX_MODULES);
        return ANCIBLE_ERROR;
    }
    
    // Add module to registry
    registry[registry_count].name = strdup(name);
    if (!registry[registry_count].name) {
        fprintf(stderr, "Error: Failed to allocate memory for module name\n");
        return ANCIBLE_ERROR;
    }
    
    registry[registry_count].func = func;
    registry_count++;
    
    return ANCIBLE_SUCCESS;
}

/**
 * Execute a task
 * 
 * @param context Execution context
 * @param task_idx Task index
 * @param args Task arguments
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int executor_run_task(context_t *context, int task_idx, const char *args, module_result_t *result) {
    if (!context || !result) {
        return ANCIBLE_ERROR;
    }
    
    // Get module name from context
    if (task_idx < 0 || task_idx >= context->playbook->task_count) {
        fprintf(stderr, "Error: Invalid task index %d\n", task_idx);
        return ANCIBLE_ERROR;
    }
    
    const char *module_name = context->playbook->task_modules[task_idx];
    if (!module_name) {
        fprintf(stderr, "Error: No module specified for task %d\n", task_idx);
        return ANCIBLE_ERROR;
    }
    
    // Find module in registry
    module_func_t module_func = NULL;
    for (int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i].name, module_name) == 0) {
            module_func = registry[i].func;
            break;
        }
    }
    
    if (!module_func) {
        fprintf(stderr, "Error: Module '%s' not found\n", module_name);
        return ANCIBLE_ERROR;
    }
    
    // Execute module
    return module_func(context, args, result);
}

/**
 * Clean up the module registry
 */
void executor_cleanup(void) {
    for (int i = 0; i < registry_count; i++) {
        free((void *)registry[i].name);
    }
    
    memset(registry, 0, sizeof(registry));
    registry_count = 0;
}
