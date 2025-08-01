#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/ancible.h"
#include "../../include/core/inventory.h"
#include "../../include/core/parser.h"
#include "../../include/core/context.h"

/**
 * Create a test host
 */
static host_t *create_test_host(void) {
    host_t *host = malloc(sizeof(host_t));
    assert(host != NULL);
    
    host->name = strdup("test_host");
    host->ansible_host = strdup("192.168.1.100");
    host->next = NULL;
    
    return host;
}

/**
 * Free a test host
 */
static void free_test_host(host_t *host) {
    if (!host) return;
    
    free(host->name);
    free(host->ansible_host);
    free(host);
}

/**
 * Create a test playbook
 */
static playbook_t *create_test_playbook(void) {
    playbook_t *playbook = malloc(sizeof(playbook_t));
    assert(playbook != NULL);
    
    playbook->hosts = strdup("all");
    playbook->task_count = 1;
    
    playbook->tasks = malloc(sizeof(task_t));
    
    playbook->tasks[0].name = strdup("Test task");
    playbook->tasks[0].module = strdup("command");
    
    return playbook;
}

/**
 * Free a test playbook
 */
static void free_test_playbook(playbook_t *playbook) {
    if (!playbook) return;
    
    free(playbook->hosts);
    
    for (int i = 0; i < playbook->task_count; i++) {
        free(playbook->tasks[i].name);
        free(playbook->tasks[i].module);
    }
    
    free(playbook->tasks);
    free(playbook);
}

/**
 * Test for context.c functionality
 */
int main(void) {
    printf("Running context.c tests\n");
    
    // Test 1: Create context
    {
        printf("Test 1: Creating context... ");
        
        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        
        context_t *context = context_create(host, playbook, 1);
        assert(context != NULL);
        assert(context->host == host);
        assert(context->playbook == playbook);
        assert(context->verbose == 1);
        
        // Check that ansible_host was set
        const char *ansible_host = context_get_var(context, "ansible_host");
        assert(ansible_host != NULL);
        assert(strcmp(ansible_host, "192.168.1.100") == 0);
        
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);
        
        printf("OK\n");
    }
    
    // Test 2: Set and get variables
    {
        printf("Test 2: Setting and getting variables... ");
        
        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        
        // Set a new variable
        int result = context_set_var(context, "test_var", "test_value");
        assert(result == ANCIBLE_SUCCESS);
        
        // Get the variable
        const char *value = context_get_var(context, "test_var");
        assert(value != NULL);
        assert(strcmp(value, "test_value") == 0);
        
        // Update the variable
        result = context_set_var(context, "test_var", "new_value");
        assert(result == ANCIBLE_SUCCESS);
        
        // Get the updated variable
        value = context_get_var(context, "test_var");
        assert(value != NULL);
        assert(strcmp(value, "new_value") == 0);
        
        // Get a non-existent variable
        value = context_get_var(context, "non_existent");
        assert(value == NULL);
        
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);
        
        printf("OK\n");
    }
    
    printf("All context.c tests passed!\n");
    return 0;
}
