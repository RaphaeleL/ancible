#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/ancible.h"
#include "../include/core/condition.h"

/**
 * Check if a string is "true" or "false"
 * 
 * @param str String to check
 * @return 1 for true, 0 for false, -1 if not a boolean
 */
static int is_boolean(const char *str) {
    if (!str) {
        return -1;
    }
    
    if (strcasecmp(str, "true") == 0 || strcasecmp(str, "yes") == 0 || strcmp(str, "1") == 0) {
        return 1;
    }
    
    if (strcasecmp(str, "false") == 0 || strcasecmp(str, "no") == 0 || strcmp(str, "0") == 0) {
        return 0;
    }
    
    return -1;
}

/**
 * Compare two strings
 * 
 * @param left Left operand
 * @param right Right operand
 * @param op Operator (==, !=, >, <, >=, <=)
 * @return 1 if comparison is true, 0 if false, -1 on error
 */
static int compare_strings(const char *left, const char *right, const char *op) {
    if (!left || !right || !op) {
        return -1;
    }
    
    if (strcmp(op, "==") == 0) {
        return strcmp(left, right) == 0 ? 1 : 0;
    } else if (strcmp(op, "!=") == 0) {
        return strcmp(left, right) != 0 ? 1 : 0;
    } else {
        // Try to convert to integers for numeric comparisons
        char *end_left, *end_right;
        long left_val = strtol(left, &end_left, 10);
        long right_val = strtol(right, &end_right, 10);
        
        // Check if both are valid integers
        if (*end_left == '\0' && *end_right == '\0') {
            if (strcmp(op, ">") == 0) {
                return left_val > right_val ? 1 : 0;
            } else if (strcmp(op, "<") == 0) {
                return left_val < right_val ? 1 : 0;
            } else if (strcmp(op, ">=") == 0) {
                return left_val >= right_val ? 1 : 0;
            } else if (strcmp(op, "<=") == 0) {
                return left_val <= right_val ? 1 : 0;
            }
        }
    }
    
    return -1;
}

/**
 * Evaluate a condition string
 * 
 * @param context Execution context
 * @param condition Condition string to evaluate
 * @return 1 if condition is true, 0 if false, -1 on error
 */
int condition_evaluate(context_t *context, const char *condition) {
    if (!context || !condition) {
        return -1;
    }
    
    // Trim leading/trailing whitespace
    while (isspace(*condition)) condition++;
    
    // Make a copy of the condition that we can modify
    char *cond_copy = strdup(condition);
    if (!cond_copy) {
        return -1;
    }
    
    // Trim trailing whitespace
    size_t len = strlen(cond_copy);
    while (len > 0 && isspace(cond_copy[len - 1])) {
        cond_copy[--len] = '\0';
    }
    
    // Remove surrounding quotes if the entire condition is quoted
    if (len >= 2 && cond_copy[0] == '"' && cond_copy[len-1] == '"') {
        cond_copy[len-1] = '\0';
        memmove(cond_copy, cond_copy + 1, len);
        len -= 2;
    }
    
    int result = -1;
    
    // Check for simple boolean values
    result = is_boolean(cond_copy);
    if (result != -1) {
        free(cond_copy);
        return result;
    }
    
    // Check for variable references
    if (cond_copy[0] == '$' || cond_copy[0] == '{') {
        // Extract variable name
        char *var_name = cond_copy;
        if (var_name[0] == '$') var_name++;
        if (var_name[0] == '{') {
            var_name++;
            char *end = strchr(var_name, '}');
            if (end) *end = '\0';
        }
        
        // Look up variable
        const char *var_value = context_get_var(context, var_name);
        if (var_value) {
            result = is_boolean(var_value);
            if (result == -1) {
                // If not a boolean, non-empty string is true
                result = *var_value != '\0' ? 1 : 0;
            }
        } else {
            // Variable not found, treat as false
            result = 0;
        }
        
        free(cond_copy);
        return result;
    }
    
    // Check for comparisons (==, !=, >, <, >=, <=)
    const char *operators[] = {"==", "!=", ">=", "<=", ">", "<"};
    for (int i = 0; i < 6; i++) {
        char *op_pos = strstr(cond_copy, operators[i]);
        if (op_pos) {
            // Split into left and right operands
            *op_pos = '\0';
            char *left = cond_copy;
            char *right = op_pos + strlen(operators[i]);
            
            // Trim whitespace
            while (isspace(*left)) left++;
            len = strlen(left);
            while (len > 0 && isspace(left[len - 1])) {
                left[--len] = '\0';
            }
            
            while (isspace(*right)) right++;
            len = strlen(right);
            while (len > 0 && isspace(right[len - 1])) {
                right[--len] = '\0';
            }
            
            // Check for variable references in operands
            if (left[0] == '$' || left[0] == '{') {
                char *var_name = left;
                if (var_name[0] == '$') var_name++;
                if (var_name[0] == '{') {
                    var_name++;
                    char *end = strchr(var_name, '}');
                    if (end) *end = '\0';
                }
                
                const char *var_value = context_get_var(context, var_name);
                if (var_value) {
                    left = (char *)var_value;
                } else {
                    left = "";
                }
            }
            
            if (right[0] == '$' || right[0] == '{') {
                char *var_name = right;
                if (var_name[0] == '$') var_name++;
                if (var_name[0] == '{') {
                    var_name++;
                    char *end = strchr(var_name, '}');
                    if (end) *end = '\0';
                }
                
                const char *var_value = context_get_var(context, var_name);
                if (var_value) {
                    right = (char *)var_value;
                } else {
                    right = "";
                }
            }
            
            // Remove surrounding quotes if present
            if (left[0] == '"' && left[strlen(left)-1] == '"') {
                left[strlen(left)-1] = '\0';
                left++;
            }
            if (right[0] == '"' && right[strlen(right)-1] == '"') {
                right[strlen(right)-1] = '\0';
                right++;
            }
            
            // Debug print
            // printf("Comparing: '%s' %s '%s'\n", left, operators[i], right);
            
            // Compare operands
            result = compare_strings(left, right, operators[i]);
            break;
        }
    }
    
    free(cond_copy);
    return result;
}
