#ifndef ANCIBLE_COMMAND_MODULE_H
#define ANCIBLE_COMMAND_MODULE_H

#include "../core/parser.h"
#include "../core/context.h"
#include "module.h"

/**
 * Execute the command module
 * 
 * @param context Execution context
 * @param args String containing module arguments
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int command_module_exec(context_t *context, const char *args, module_result_t *result);

#endif /* ANCIBLE_COMMAND_MODULE_H */
