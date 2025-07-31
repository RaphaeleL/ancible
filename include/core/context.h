#ifndef ANCIBLE_CONTEXT_H
#define ANCIBLE_CONTEXT_H

#include "inventory.h"
#include "parser.h"

/**
 * Structure to hold a variable
 */
typedef struct variable {
    char *name;           // Variable name
    char *value;          // Variable value
    struct variable *next; // Next variable in the list
} variable_t;

/**
 * Structure to hold execution context for a host
 */
typedef struct {
    host_t *host;         // Host to execute on
    playbook_t *playbook; // Playbook to execute
    variable_t *vars;     // Variables for this host
    int verbose;          // Whether to be verbose
} context_t;

/**
 * Create a new execution context
 * 
 * @param host Host to execute on
 * @param playbook Playbook to execute
 * @param verbose Whether to be verbose
 * @return Pointer to the new context, or NULL on error
 */
context_t *context_create(host_t *host, playbook_t *playbook, int verbose);

/**
 * Free resources used by a context
 * 
 * @param context Pointer to context to free
 */
void context_free(context_t *context);

/**
 * Set a variable in the context
 * 
 * @param context Pointer to the context
 * @param name Variable name
 * @param value Variable value
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int context_set_var(context_t *context, const char *name, const char *value);

/**
 * Get a variable from the context
 * 
 * @param context Pointer to the context
 * @param name Variable name
 * @return Variable value, or NULL if not found
 */
const char *context_get_var(context_t *context, const char *name);

/**
 * Print context (for debugging)
 * 
 * @param context Pointer to context to print
 */
void context_print(const context_t *context);

#endif /* ANCIBLE_CONTEXT_H */
