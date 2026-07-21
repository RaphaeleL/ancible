#ifndef ANCIBLE_FILE_MODULE_H
#define ANCIBLE_FILE_MODULE_H

#include "../core/parser.h"
#include "../core/context.h"
#include "module.h"

/**
 * Execute the file module
 *
 * Manages files and directories on the target host:
 *   - state=present: ensure file exists
 *   - state=absent:  remove file or directory
 *   - state=touch:   ensure file exists, update timestamp
 *   - state=directory: ensure directory exists
 *   - mode=<octal>:  set permissions (e.g. 0644)
 *
 * @param context Execution context
 * @param args String containing module arguments
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int file_module_exec(context_t *context, const char *args, module_result_t *result);

#endif /* ANCIBLE_FILE_MODULE_H */
