#ifndef ANCIBLE_COPY_MODULE_H
#define ANCIBLE_COPY_MODULE_H

#include "../core/parser.h"
#include "../core/context.h"
#include "module.h"

/**
 * Execute the copy module
 *
 * Copies content to a destination path on the target host:
 *   - src=<path> dest=<path>: copy a local source file to dest
 *   - content=<text> dest=<path>: write literal content to dest
 *   - mode=<octal>: set permissions on dest (e.g. 0644)
 *   - force=yes|no: overwrite dest when content differs (default yes)
 *
 * @param context Execution context
 * @param args String containing module arguments
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int copy_module_exec(context_t *context, const char *args, module_result_t *result);

#endif /* ANCIBLE_COPY_MODULE_H */
