#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/ancible.h"
#include "../../include/core/context.h"
#include "../../include/core/executor.h"
#include "../../include/modules/module.h"

/**
 * Create a test host
 */
static host_t *create_test_host(void) {
    host_t *host = malloc(sizeof(host_t));
    assert(host != NULL);
    
    host->name = strdup("localhost");
    host->ansible_host = strdup("localhost");
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
    
    playbook->tasks[0].name = strdup("Test command");
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
 * Test module function
 */
static int test_module_func(context_t *context, const char *args __attribute__((unused)), module_result_t *result) {
    if (!context || !result) {
        return ANCIBLE_ERROR;
    }
    
    module_result_init(result);
    
    result->changed = 1;
    result->msg = strdup("Test module executed");
    
    return ANCIBLE_SUCCESS;
}

/**
 * Test for executor functionality
 */
int main(void) {
    printf("Running executor tests\n");
    
    // Test 1: Initialize executor
    {
        printf("Test 1: Initializing executor... ");
        
        int ret = executor_init();
        assert(ret == ANCIBLE_SUCCESS);
        
        printf("OK\n");
    }
    
    // Test 2: Register a custom module
    {
        printf("Test 2: Registering custom module... ");
        
        int ret = executor_register_module("test", test_module_func);
        assert(ret == ANCIBLE_SUCCESS);
        
        printf("OK\n");
    }
    
    // Test 3: Execute command module
    {
        printf("Test 3: Executing command module... ");
        
        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        
        // Set local connection
        context_set_var(context, "ansible_connection", "local");
        
        // Execute module
        module_result_t result;
        int ret = executor_run_task(context, 0, "echo 'Hello from executor!'", &result);
        
        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 1);
        assert(result.cmd_result.exit_code == 0);
        assert(result.cmd_result.stdout_data != NULL);
        assert(strstr(result.cmd_result.stdout_data, "Hello from executor!") != NULL);
        
        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);
        
        printf("OK\n");
    }
    
    // Test 4: Clean up
    {
        printf("Test 4: Cleaning up executor... ");
        
        executor_cleanup();
        
        printf("OK\n");
    }
    
    printf("All executor tests passed!\n");
    return 0;
}
