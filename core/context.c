#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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

static int append_to_buffer(char **buffer, size_t *length, size_t *capacity,
    const char *text, size_t text_len) {
    if (!buffer || !length || !capacity || (!text && text_len > 0)) {
        return ANCIBLE_ERROR;
    }

    size_t needed = *length + text_len + 1;
    if (needed > *capacity) {
        size_t new_capacity = *capacity ? *capacity : 64;
        while (new_capacity < needed) {
            new_capacity *= 2;
        }

        char *new_buffer = realloc(*buffer, new_capacity);
        if (!new_buffer) {
            return ANCIBLE_ERROR;
        }

        *buffer = new_buffer;
        *capacity = new_capacity;
    }

    if (text_len > 0) {
        memcpy(*buffer + *length, text, text_len);
        *length += text_len;
    }

    (*buffer)[*length] = '\0';
    return ANCIBLE_SUCCESS;
}

static char *strndup_trimmed(const char *start, size_t len) {
    if (!start) {
        return NULL;
    }

    while (len > 0 && isspace((unsigned char)*start)) {
        start++;
        len--;
    }
    while (len > 0 && isspace((unsigned char)start[len - 1])) {
        len--;
    }

    char *out = malloc(len + 1);
    if (!out) {
        return NULL;
    }
    memcpy(out, start, len);
    out[len] = '\0';
    return out;
}

static char *strdup_lower(const char *src) {
    size_t len = strlen(src);
    char *out = strdup(src);
    if (!out) {
        return NULL;
    }
    for (size_t i = 0; i < len; i++) {
        out[i] = (char)tolower((unsigned char)out[i]);
    }
    return out;
}

static char *strdup_upper(const char *src) {
    size_t len = strlen(src);
    char *out = strdup(src);
    if (!out) {
        return NULL;
    }
    for (size_t i = 0; i < len; i++) {
        out[i] = (char)toupper((unsigned char)out[i]);
    }
    return out;
}

static char *apply_filter(const char *current, const char *filter) {
    if (!current || !filter) {
        return NULL;
    }

    if (strcmp(filter, "trim") == 0) {
        return strndup_trimmed(current, strlen(current));
    }
    if (strcmp(filter, "lower") == 0) {
        return strdup_lower(current);
    }
    if (strcmp(filter, "upper") == 0) {
        return strdup_upper(current);
    }
    if (strcmp(filter, "length") == 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%zu", strlen(current));
        return strdup(buf);
    }

    if (strncmp(filter, "default(", 8) == 0) {
        size_t len = strlen(filter);
        if (len >= 9 && filter[len - 1] == ')') {
            char *arg = strndup_trimmed(filter + 8, len - 9);
            if (!arg) {
                return NULL;
            }

            size_t arg_len = strlen(arg);
            if (arg_len >= 2 &&
                ((arg[0] == '"' && arg[arg_len - 1] == '"') ||
                 (arg[0] == '\'' && arg[arg_len - 1] == '\''))) {
                memmove(arg, arg + 1, arg_len - 2);
                arg[arg_len - 2] = '\0';
            }

            char *out = NULL;
            if (current[0] == '\0') {
                out = strdup(arg);
            } else {
                out = strdup(current);
            }
            free(arg);
            return out;
        }
    }

    // Unknown filter: keep value unchanged.
    return strdup(current);
}

static char *evaluate_template_expression(context_t *context, const char *expr, size_t expr_len) {
    char *work = strndup_trimmed(expr, expr_len);
    if (!work) {
        return NULL;
    }

    // Split on "|" for simple filter chains.
    char *saveptr = NULL;
    char *token = strtok_r(work, "|", &saveptr);
    if (!token) {
        free(work);
        return strdup("");
    }

    char *base = strndup_trimmed(token, strlen(token));
    if (!base) {
        free(work);
        return NULL;
    }

    const char *value = context_get_var(context, base);
    char *current = strdup(value ? value : "");
    free(base);
    if (!current) {
        free(work);
        return NULL;
    }

    while ((token = strtok_r(NULL, "|", &saveptr)) != NULL) {
        char *filter = strndup_trimmed(token, strlen(token));
        if (!filter) {
            free(current);
            free(work);
            return NULL;
        }

        char *next = apply_filter(current, filter);
        free(filter);
        free(current);
        if (!next) {
            free(work);
            return NULL;
        }
        current = next;
    }

    free(work);
    return current;
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

char *context_render_template(context_t *context, const char *input) {
    if (!context || !input) {
        return NULL;
    }

    char *out = NULL;
    size_t out_len = 0;
    size_t out_cap = 0;

    const char *cursor = input;
    while (*cursor) {
        const char *open = strstr(cursor, "{{");
        if (!open) {
            if (append_to_buffer(&out, &out_len, &out_cap, cursor, strlen(cursor)) != ANCIBLE_SUCCESS) {
                free(out);
                return NULL;
            }
            break;
        }

        if (append_to_buffer(&out, &out_len, &out_cap, cursor, (size_t)(open - cursor)) != ANCIBLE_SUCCESS) {
            free(out);
            return NULL;
        }

        const char *close = strstr(open + 2, "}}");
        if (!close) {
            if (append_to_buffer(&out, &out_len, &out_cap, open, strlen(open)) != ANCIBLE_SUCCESS) {
                free(out);
                return NULL;
            }
            break;
        }

        char *rendered_expr = evaluate_template_expression(context, open + 2, (size_t)(close - (open + 2)));
        if (!rendered_expr) {
            free(out);
            return NULL;
        }
        if (append_to_buffer(&out, &out_len, &out_cap, rendered_expr, strlen(rendered_expr)) != ANCIBLE_SUCCESS) {
            free(rendered_expr);
            free(out);
            return NULL;
        }
        free(rendered_expr);

        cursor = close + 2;
    }

    if (!out) {
        out = strdup("");
    }
    return out;
}

char *context_eval_expression(context_t *context, const char *expression) {
    if (!context || !expression) {
        return NULL;
    }
    return evaluate_template_expression(context, expression, strlen(expression));
}

int context_apply_register(context_t *context, const char *basename,
    const char *stdout_data, const char *stderr_data, int exit_code,
    int failed, const char *msg) {
    if (!context || !basename || !*basename) {
        return ANCIBLE_ERROR;
    }

    char key[256];
    char rcbuf[32];

    if (snprintf(key, sizeof(key), "%s.stdout", basename) >= (int)sizeof(key)) {
        return ANCIBLE_ERROR;
    }
    if (context_set_var(context, key, stdout_data ? stdout_data : "") != ANCIBLE_SUCCESS) {
        return ANCIBLE_ERROR;
    }

    if (snprintf(key, sizeof(key), "%s.stderr", basename) >= (int)sizeof(key)) {
        return ANCIBLE_ERROR;
    }
    if (context_set_var(context, key, stderr_data ? stderr_data : "") != ANCIBLE_SUCCESS) {
        return ANCIBLE_ERROR;
    }

    if (snprintf(key, sizeof(key), "%s.rc", basename) >= (int)sizeof(key)) {
        return ANCIBLE_ERROR;
    }
    snprintf(rcbuf, sizeof(rcbuf), "%d", exit_code);
    if (context_set_var(context, key, rcbuf) != ANCIBLE_SUCCESS) {
        return ANCIBLE_ERROR;
    }

    if (snprintf(key, sizeof(key), "%s.failed", basename) >= (int)sizeof(key)) {
        return ANCIBLE_ERROR;
    }
    if (context_set_var(context, key, failed ? "true" : "false") != ANCIBLE_SUCCESS) {
        return ANCIBLE_ERROR;
    }

    if (snprintf(key, sizeof(key), "%s.msg", basename) >= (int)sizeof(key)) {
        return ANCIBLE_ERROR;
    }
    if (context_set_var(context, key, msg ? msg : "") != ANCIBLE_SUCCESS) {
        return ANCIBLE_ERROR;
    }

    return ANCIBLE_SUCCESS;
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
