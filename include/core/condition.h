#ifndef ANCIBLE_CONDITION_H
#define ANCIBLE_CONDITION_H

#include "context.h"

/**
 * Evaluate a condition string
 * 
 * @param context Execution context
 * @param condition Condition string to evaluate
 * @return 1 if condition is true, 0 if false, -1 on error
 */
int condition_evaluate(context_t *context, const char *condition);

#endif /* ANCIBLE_CONDITION_H */
