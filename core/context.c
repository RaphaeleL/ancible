#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ancible.h"
#include "../include/core/context.h"

/**
 * Create a new variable
 * 
 * @param name Variable name
 * @param value Variable value
 * @return Pointer to the new variable, or NULL on error
 */
static variable_t *variable_create(const char *name, const char *value) {
    variable_t *var = malloc(sizeof(variable_t));
    if (!var) {
        fprintf(stderr, "Error: Failed to allocate memory for variable\n");
        return NULL;
    }
    
    var->name = strdup(name);
    if (!var->name) {
        fprintf(stderr, "Error: Failed to allocate memory for variable name\n");
        free(var);
        return NULL;
    }
    
    var->value = strdup(value);
    if (!var->value) {
        fprintf(stderr, "Error: Failed to allocate memory for variable value\n");
        free(var->name);
        free(var);
        return NULL;
    }
    
    var->next = NULL;
    
    return var;
}

/**
 * Free a variable
 * 
 * @param var Pointer to variable to free
 */
static void variable_free(variable_t *var) {
    if (!var) {
        return;
    }
    
    free(var->name);
    free(var->value);
    free(var);
}

/**
 * Create a new execution context
 * 
 * @param host Host to execute on
 * @param playbook Playbook to execute
 * @param verbose Whether to be verbose
 * @return Pointer to the new context, or NULL on error
 */
context_t *context_create(host_t *host, playbook_t *playbook, int verbose) {
    if (!host || !playbook) {
        fprintf(stderr, "Error: Host and playbook are required for context\n");
        return NULL;
    }
    
    context_t *context = malloc(sizeof(context_t));
    if (!context) {
        fprintf(stderr, "Error: Failed to allocate memory for context\n");
        return NULL;
    }
    
    context->host = host;
    context->playbook = playbook;
    context->vars = NULL;
    context->verbose = verbose;
    
    // Set default variables
    context_set_var(context, "ansible_host", host->ansible_host ? host->ansible_host : host->name);
    
    // Default connection type is ssh
    context_set_var(context, "ansible_connection", "ssh");
    
    return context;
}

/**
 * Free resources used by a context
 * 
 * @param context Pointer to context to free
 */
void context_free(context_t *context) {
    if (!context) {
        return;
    }
    
    // Free variables
    variable_t *var = context->vars;
    while (var) {
        variable_t *next = var->next;
        variable_free(var);
        var = next;
    }
    
    // We don't free host or playbook, as they are owned by the inventory and parser
    
    free(context);
}

/**
 * Set a variable in the context
 * 
 * @param context Pointer to the context
 * @param name Variable name
 * @param value Variable value
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int context_set_var(context_t *context, const char *name, const char *value) {
    if (!context || !name || !value) {
        return ANCIBLE_ERROR;
    }
    
    // Check if variable already exists
    variable_t *var = context->vars;
    while (var) {
        if (strcmp(var->name, name) == 0) {
            // Update existing variable
            char *new_value = strdup(value);
            if (!new_value) {
                fprintf(stderr, "Error: Failed to allocate memory for variable value\n");
                return ANCIBLE_ERROR;
            }
            
            free(var->value);
            var->value = new_value;
            return ANCIBLE_SUCCESS;
        }
        var = var->next;
    }
    
    // Create new variable
    variable_t *new_var = variable_create(name, value);
    if (!new_var) {
        return ANCIBLE_ERROR;
    }
    
    // Add to the beginning of the list
    new_var->next = context->vars;
    context->vars = new_var;
    
    return ANCIBLE_SUCCESS;
}

/**
 * Get a variable from the context
 * 
 * @param context Pointer to the context
 * @param name Variable name
 * @return Variable value, or NULL if not found
 */
const char *context_get_var(context_t *context, const char *name) {
    if (!context || !name) {
        return NULL;
    }
    
    variable_t *var = context->vars;
    while (var) {
        if (strcmp(var->name, name) == 0) {
            return var->value;
        }
        var = var->next;
    }
    
    return NULL;
}

/**
 * Print context (for debugging)
 * 
 * @param context Pointer to context to print
 */
void context_print(const context_t *context) {
    if (!context) {
        return;
    }
    
    printf("Context:\n");
    printf("  Host: %s\n", context->host->name);
    if (context->host->ansible_host) {
        printf("  Ansible Host: %s\n", context->host->ansible_host);
    }
    
    printf("  Variables:\n");
    variable_t *var = context->vars;
    while (var) {
        printf("    %s: %s\n", var->name, var->value);
        var = var->next;
    }
}
