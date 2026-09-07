#ifndef ANCIBLE_TEMPLATE_MODULE_H
#define ANCIBLE_TEMPLATE_MODULE_H

#include "module.h"

/**
 * Render a local template file and write it to the destination.
 *
 * Supported arguments are src=, dest=, and optional mode= (octal).
 */
int template_module_exec(context_t *context, const char *args, module_result_t *result);

#endif /* ANCIBLE_TEMPLATE_MODULE_H */
