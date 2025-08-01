#ifndef ANCIBLE_EXECUTOR_H
#define ANCIBLE_EXECUTOR_H

#include "../modules/module.h"
#include "context.h"

/**
 * Module registry entry
 */
typedef struct {
    const char *name;
    module_func_t func;
} module_registry_entry_t;

/**
 * Initialize the module registry
 * 
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int executor_init(void);

/**
 * Register a module
 * 
 * @param name Module name
 * @param func Module function
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int executor_register_module(const char *name, module_func_t func);

/**
 * Execute a task
 * 
 * @param context Execution context
 * @param task_idx Task index
 * @param args Task arguments
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int executor_run_task(context_t *context, int task_idx, const char *args, module_result_t *result);

/**
 * Execute a block of tasks
 * 
 * @param context Execution context
 * @param block_idx Block task index
 * @param args Task arguments
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int executor_run_block(context_t *context, int block_idx, const char *args, module_result_t *result);

/**
 * Clean up the module registry
 */
void executor_cleanup(void);

#endif /* ANCIBLE_EXECUTOR_H */
