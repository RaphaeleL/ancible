#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <strings.h> // For strcasecmp
#include "../../include/ancible.h"
#include "../../include/core/condition.h"
#include "../../include/core/context.h"
#include "../../include/core/inventory.h" // For host_t
#include "../../include/core/parser.h" // For playbook_t

/**
 * Create a minimal test context for testing
 */
context_t *create_test_context(void) {
    // Create a dummy host
    host_t *host = malloc(sizeof(host_t));
    memset(host, 0, sizeof(host_t));
    host->name = strdup("test-host");
    
    // Create a dummy playbook
    playbook_t *playbook = malloc(sizeof(playbook_t));
    memset(playbook, 0, sizeof(playbook_t));
    playbook->hosts = strdup("all");
    
    // Create the context
    context_t *context = context_create(host, playbook, 1);
    return context;
}

/**
 * Free a test context including its host and playbook
 */
void free_test_context(context_t *context) {
    if (!context) return;
    
    // Save pointers to host and playbook before freeing context
    host_t *host = context->host;
    playbook_t *playbook = context->playbook;
    
    // Free context
    context_free(context);
    
    // Free host
    if (host) {
        free(host->name);
        free(host);
    }
    
    // Free playbook
    if (playbook) {
        free(playbook->hosts);
        free(playbook);
    }
}

/**
 * Test simple boolean conditions
 */
void test_boolean_conditions(void) {
    // Create a test context
    context_t *context = create_test_context();
    assert(context != NULL);
    
    // Test true conditions
    assert(condition_evaluate(context, "true") == 1);
    assert(condition_evaluate(context, "TRUE") == 1);
    assert(condition_evaluate(context, "yes") == 1);
    assert(condition_evaluate(context, "YES") == 1);
    assert(condition_evaluate(context, "1") == 1);
    
    // Test false conditions
    assert(condition_evaluate(context, "false") == 0);
    assert(condition_evaluate(context, "FALSE") == 0);
    assert(condition_evaluate(context, "no") == 0);
    assert(condition_evaluate(context, "NO") == 0);
    assert(condition_evaluate(context, "0") == 0);
    
    // Clean up
    free_test_context(context);
    printf("Boolean conditions test passed\n");
}

/**
 * Test comparison conditions
 */
void test_comparison_conditions(void) {
    // Create a test context
    context_t *context = create_test_context();
    assert(context != NULL);
    
    // Test string comparisons
    assert(condition_evaluate(context, "abc == abc") == 1);
    assert(condition_evaluate(context, "abc != def") == 1);
    assert(condition_evaluate(context, "abc == def") == 0);
    assert(condition_evaluate(context, "abc != abc") == 0);
    
    // Test numeric comparisons
    assert(condition_evaluate(context, "123 == 123") == 1);
    assert(condition_evaluate(context, "123 != 456") == 1);
    assert(condition_evaluate(context, "123 < 456") == 1);
    assert(condition_evaluate(context, "456 > 123") == 1);
    assert(condition_evaluate(context, "123 <= 123") == 1);
    assert(condition_evaluate(context, "123 >= 123") == 1);
    assert(condition_evaluate(context, "456 < 123") == 0);
    assert(condition_evaluate(context, "123 > 456") == 0);
    
    // Clean up
    free_test_context(context);
    printf("Comparison conditions test passed\n");
}

/**
 * Main test function
 */
int main(void) {
    printf("Running condition tests...\n");
    
    test_boolean_conditions();
    test_comparison_conditions();
    
    printf("All condition tests passed!\n");
    return 0;
}
