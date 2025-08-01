#ifndef ANCIBLE_MODULE_H
#define ANCIBLE_MODULE_H

#include "../core/parser.h"
#include "../core/context.h"
#include "../transport/runner.h"

/**
 * Structure to hold module result
 */
typedef struct {
    int changed;         // Whether the module made changes
    int failed;          // Whether the module failed
    int skipped;         // Whether the module was skipped
    char *msg;           // Message from the module
    command_result_t cmd_result;  // Command result (if applicable)
} module_result_t;

/**
 * Module function signature
 * 
 * @param context Execution context
 * @param args String containing module arguments
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
typedef int (*module_func_t)(context_t *context, const char *args, module_result_t *result);

/**
 * Initialize module result structure
 * 
 * @param result Pointer to result structure to initialize
 */
void module_result_init(module_result_t *result);

/**
 * Free resources used by a module result
 * 
 * @param result Pointer to result structure to free
 */
void module_result_free(module_result_t *result);

/**
 * Print module result (for debugging)
 * 
 * @param result Pointer to result structure to print
 */
void module_result_print(const module_result_t *result);

#endif /* ANCIBLE_MODULE_H */
