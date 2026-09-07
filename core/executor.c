#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ancible.h"
#include "../include/core/executor.h"
#include "../include/core/condition.h"
#include "../include/modules/command.h"
#include "../include/modules/file.h"
#include "../include/modules/copy.h"
#include "../include/modules/template.h"

#define MAX_MODULES 32

// Module registry
static module_registry_entry_t registry[MAX_MODULES];
static int registry_count = 0;

static const char *render_task_label(context_t *context, const char *name, char **owned_rendered) {
    if (owned_rendered) {
        *owned_rendered = NULL;
    }
    if (!name) {
        return "unnamed";
    }
    if (!context || !strstr(name, "{{")) {
        return name;
    }
    char *rendered = context_render_template(context, name);
    if (!rendered) {
        return name;
    }
    if (owned_rendered) {
        *owned_rendered = rendered;
    }
    return rendered;
}

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
    
    if (executor_register_module("file", file_module_exec) != ANCIBLE_SUCCESS) {
        fprintf(stderr, "Error: Failed to register file module\n");
        return ANCIBLE_ERROR;
    }

    if (executor_register_module("copy", copy_module_exec) != ANCIBLE_SUCCESS) {
        fprintf(stderr, "Error: Failed to register copy module\n");
        return ANCIBLE_ERROR;
    }

    if (executor_register_module("template", template_module_exec) != ANCIBLE_SUCCESS) {
        fprintf(stderr, "Error: Failed to register template module\n");
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
    
    // Get task from context
    if (task_idx < 0 || task_idx >= context->playbook->task_count) {
        fprintf(stderr, "Error: Invalid task index %d\n", task_idx);
        return ANCIBLE_ERROR;
    }
    
    task_t *task = &context->playbook->tasks[task_idx];
    
    // Handle different task types
    if (task->type == TASK_TYPE_BLOCK) {
        return executor_run_block(context, task_idx, args, result);
    } else if (task->type == TASK_TYPE_RESCUE || task->type == TASK_TYPE_ALWAYS) {
        // These should be handled by executor_run_block, not called directly
        fprintf(stderr, "Error: Rescue and Always blocks should not be executed directly\n");
        return ANCIBLE_ERROR;
    }
    
    // This is a normal task, proceed with execution
    
    // Check if this task has a when condition
    if (task->when) {
        int condition_result = condition_evaluate(context, task->when);
        
        // If condition is false, skip this task
        if (condition_result == 0) {
            char *rendered_name = NULL;
            const char *task_name = render_task_label(context, task->name, &rendered_name);
            if (context->verbose) {
                printf("Skipping task '%s' due to condition: %s\n", 
                       task_name,
                       task->when);
            }
            free(rendered_name);
            
            // Set result to indicate skipped task
            result->changed = 0;
            result->failed = 0;
            result->skipped = 1;
            result->msg = strdup("Skipped due to condition");
            
            return ANCIBLE_SUCCESS;
        } else if (condition_result < 0) {
            fprintf(stderr, "Error: Failed to evaluate condition: %s\n", task->when);
            return ANCIBLE_ERROR;
        }
    }
    
    task->module = context->playbook->tasks[task_idx].module;
    if (!task->module) {
        fprintf(stderr, "Error: No module specified for task %d - %s\n", task_idx, task->name ? task->name : "unnamed");
        return ANCIBLE_ERROR;
    }
    
    // Find module in registry
    module_func_t module_func = NULL;
    for (int i = 0; i < registry_count; i++) {
        if (strcmp(registry[i].name, task->module) == 0) {
            module_func = registry[i].func;
            break;
        }
    }
    
    if (!module_func) {
        fprintf(stderr, "Error: Module '%s' not found\n", task->module);
        return ANCIBLE_ERROR;
    }
    
    const char *module_args = args;
    char *rendered_args = NULL;
    if (args) {
        rendered_args = context_render_template(context, args);
        if (!rendered_args) {
            fprintf(stderr, "Error: Failed to render task arguments\n");
            return ANCIBLE_ERROR;
        }
        module_args = rendered_args;
    }

    // Execute module
    int mod_rc = module_func(context, module_args, result);
    free(rendered_args);
    if (mod_rc == ANCIBLE_SUCCESS && task->register_var && result && !result->skipped) {
        context_apply_register(context, task->register_var,
            result->cmd_result.stdout_data,
            result->cmd_result.stderr_data,
            result->cmd_result.exit_code,
            result->failed,
            result->msg);
    }
    return mod_rc;
}

/**
 * Execute a block of tasks
 * 
 * @param context Execution context
 * @param block_idx Block task index
 * @param args Task arguments
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int executor_run_block(context_t *context, int block_idx, const char *args, module_result_t *result) {
    (void)args; // Suppress unused parameter warning
    if (!context || !result) {
        return ANCIBLE_ERROR;
    }
    
    // Get block task from context
    if (block_idx < 0 || block_idx >= context->playbook->task_count) {
        fprintf(stderr, "Error: Invalid block index %d\n", block_idx);
        return ANCIBLE_ERROR;
    }
    
    task_t *block = &context->playbook->tasks[block_idx];
    
    // Check if this is actually a block
    if (block->type != TASK_TYPE_BLOCK) {
        fprintf(stderr, "Error: Task %d is not a block\n", block_idx);
        return ANCIBLE_ERROR;
    }
    
    // Check if this block has a when condition
    if (block->when) {
        int condition_result = condition_evaluate(context, block->when);
        
        // If condition is false, skip this block
        if (condition_result == 0) {
            char *rendered_name = NULL;
            const char *block_name = render_task_label(context, block->name, &rendered_name);
            if (context->verbose) {
                printf("Skipping block '%s' due to condition: %s\n", 
                       block_name,
                       block->when);
            }
            free(rendered_name);
            
            // Set result to indicate skipped block
            result->changed = 0;
            result->failed = 0;
            result->skipped = 1;
            result->msg = strdup("Skipped due to condition");
            
            return ANCIBLE_SUCCESS;
        } else if (condition_result < 0) {
            fprintf(stderr, "Error: Failed to evaluate condition: %s\n", block->when);
            return ANCIBLE_ERROR;
        }
    }
    
    // Execute tasks in the block
    int block_result = ANCIBLE_SUCCESS;
    int any_failed = 0;
    
    // Find rescue and always blocks if they exist
    int rescue_idx = -1;
    int always_idx = -1;
    
    for (int i = 0; i < context->playbook->task_count; i++) {
        task_t *task = &context->playbook->tasks[i];
        
        if (task->parent_idx == block_idx) {
            if (task->type == TASK_TYPE_RESCUE) {
                rescue_idx = i;
            } else if (task->type == TASK_TYPE_ALWAYS) {
                always_idx = i;
            }
        }
    }
    
    // Execute tasks in the block
    for (int i = 0; i < block->subtask_count; i++) {
        int subtask_idx = block->subtask_indices[i];
        module_result_t subtask_result;
        
        // Initialize result
        module_result_init(&subtask_result);
        
        // Extract command arguments for this subtask from the playbook file
        char subtask_args[1024] = {0};
        task_t *subtask = &context->playbook->tasks[subtask_idx];
        
        if (subtask->module && strcmp(subtask->module, "command") == 0) {
            // For command module, we need to extract the actual command
            // This is a simplified approach - in a real implementation, we'd parse the YAML properly
            if (subtask->name) {
                // For now, create a simple command based on the task name
                snprintf(subtask_args, sizeof(subtask_args), "echo 'Executing %s'", subtask->name);
            } else {
                snprintf(subtask_args, sizeof(subtask_args), "echo 'Executing unnamed task'");
            }
        }
        
        // Execute subtask
        int subtask_res = executor_run_task(context, subtask_idx, subtask_args, &subtask_result);
        
        // Print subtask output in verbose mode
        if (context->verbose) {
            if (subtask_result.msg) {
                printf("  Message: %s\n", subtask_result.msg);
            }
            if (subtask_result.cmd_result.stdout_data && strlen(subtask_result.cmd_result.stdout_data) > 0) {
                printf("  Stdout: %s", subtask_result.cmd_result.stdout_data);
            }
            if (subtask_result.cmd_result.stderr_data && strlen(subtask_result.cmd_result.stderr_data) > 0) {
                printf("  Stderr: %s", subtask_result.cmd_result.stderr_data);
            }
        }
        
        // Check for failure
        if (subtask_res != ANCIBLE_SUCCESS || subtask_result.failed) {
            any_failed = 1;
            block_result = ANCIBLE_ERROR;
            
            // Break out of the loop, we'll handle rescue later
            module_result_free(&subtask_result);
            break;
        }
        
        // Free result
        module_result_free(&subtask_result);
    }
    
    // Handle rescue block if there was a failure
    if (any_failed && rescue_idx >= 0) {
        task_t *rescue = &context->playbook->tasks[rescue_idx];
        
        if (context->verbose) {
            char *rendered_name = NULL;
            const char *block_name = render_task_label(context, block->name, &rendered_name);
            printf("Executing rescue block for '%s'\n", 
                   block_name);
            free(rendered_name);
        }
        
        // Execute tasks in the rescue block
        for (int i = 0; i < rescue->subtask_count; i++) {
            int subtask_idx = rescue->subtask_indices[i];
            module_result_t subtask_result;
            
            // Initialize result
            module_result_init(&subtask_result);
            
            // Extract command arguments for this subtask
            char subtask_args[1024] = {0};
            task_t *subtask = &context->playbook->tasks[subtask_idx];
            
            if (subtask->module && strcmp(subtask->module, "command") == 0) {
                if (subtask->name) {
                    snprintf(subtask_args, sizeof(subtask_args), "echo 'Executing %s'", subtask->name);
                } else {
                    snprintf(subtask_args, sizeof(subtask_args), "echo 'Executing unnamed task'");
                }
            }
            
            // Execute subtask
            executor_run_task(context, subtask_idx, subtask_args, &subtask_result);
            
            // Print subtask output in verbose mode
            if (context->verbose) {
                if (subtask_result.msg) {
                    printf("  Message: %s\n", subtask_result.msg);
                }
                if (subtask_result.cmd_result.stdout_data && strlen(subtask_result.cmd_result.stdout_data) > 0) {
                    printf("  Stdout: %s", subtask_result.cmd_result.stdout_data);
                }
                if (subtask_result.cmd_result.stderr_data && strlen(subtask_result.cmd_result.stderr_data) > 0) {
                    printf("  Stderr: %s", subtask_result.cmd_result.stderr_data);
                }
            }
            
            // Free result
            module_result_free(&subtask_result);
        }
        
        // Reset block result to success since rescue handled the error
        block_result = ANCIBLE_SUCCESS;
    }
    
    // Handle always block if it exists
    if (always_idx >= 0) {
        task_t *always = &context->playbook->tasks[always_idx];
        
        if (context->verbose) {
            char *rendered_name = NULL;
            const char *block_name = render_task_label(context, block->name, &rendered_name);
            printf("Executing always block for '%s'\n", 
                   block_name);
            free(rendered_name);
        }
        
        // Execute tasks in the always block
        for (int i = 0; i < always->subtask_count; i++) {
            int subtask_idx = always->subtask_indices[i];
            module_result_t subtask_result;
            
            // Initialize result
            module_result_init(&subtask_result);
            
            // Extract command arguments for this subtask
            char subtask_args[1024] = {0};
            task_t *subtask = &context->playbook->tasks[subtask_idx];
            
            if (subtask->module && strcmp(subtask->module, "command") == 0) {
                if (subtask->name) {
                    snprintf(subtask_args, sizeof(subtask_args), "echo 'Executing %s'", subtask->name);
                } else {
                    snprintf(subtask_args, sizeof(subtask_args), "echo 'Executing unnamed task'");
                }
            }
            
            // Execute subtask
            executor_run_task(context, subtask_idx, subtask_args, &subtask_result);
            
            // Print subtask output in verbose mode
            if (context->verbose) {
                if (subtask_result.msg) {
                    printf("  Message: %s\n", subtask_result.msg);
                }
                if (subtask_result.cmd_result.stdout_data && strlen(subtask_result.cmd_result.stdout_data) > 0) {
                    printf("  Stdout: %s", subtask_result.cmd_result.stdout_data);
                }
                if (subtask_result.cmd_result.stderr_data && strlen(subtask_result.cmd_result.stderr_data) > 0) {
                    printf("  Stderr: %s", subtask_result.cmd_result.stderr_data);
                }
            }
            
            // Free result
            module_result_free(&subtask_result);
        }
    }
    
    // Set result
    result->changed = 0;  // We can't determine if anything changed from the block
    result->failed = (block_result != ANCIBLE_SUCCESS);
    result->skipped = 0;
    
    if (result->failed) {
        result->msg = strdup("Block execution failed");
    } else {
        result->msg = strdup("Block executed successfully");
    }
    
    return block_result;
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
