#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/ancible.h"
#include "../../include/core/context.h"
#include "../../include/modules/module.h"
#include "../../include/modules/command.h"

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
    
    playbook->task_names = malloc(sizeof(char *));
    playbook->task_modules = malloc(sizeof(char *));
    
    playbook->task_names[0] = strdup("Test task");
    playbook->task_modules[0] = strdup("command");
    
    return playbook;
}

/**
 * Free a test playbook
 */
static void free_test_playbook(playbook_t *playbook) {
    if (!playbook) return;
    
    free(playbook->hosts);
    
    for (int i = 0; i < playbook->task_count; i++) {
        free(playbook->task_names[i]);
        free(playbook->task_modules[i]);
    }
    
    free(playbook->task_names);
    free(playbook->task_modules);
    
    free(playbook);
}

/**
 * Test for command module functionality
 */
int main(void) {
    printf("Running command module tests\n");
    
    // Test 1: Execute simple command
    {
        printf("Test 1: Executing simple command... ");
        
        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        
        // Set local connection
        context_set_var(context, "ansible_connection", "local");
        
        // Execute module
        module_result_t result;
        int ret = command_module_exec(context, "echo 'Hello from command module!'", &result);
        
        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 1);
        assert(result.cmd_result.exit_code == 0);
        assert(result.cmd_result.stdout_data != NULL);
        assert(strstr(result.cmd_result.stdout_data, "Hello from command module!") != NULL);
        
        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);
        
        printf("OK\n");
    }
    
    // Test 2: Execute failing command
    {
        printf("Test 2: Executing failing command... ");
        
        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        
        // Set local connection
        context_set_var(context, "ansible_connection", "local");
        
        // Execute module
        module_result_t result;
        int ret = command_module_exec(context, "exit 1", &result);
        
        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 1);
        assert(result.cmd_result.exit_code == 1);
        
        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);
        
        printf("OK\n");
    }
    
    printf("All command module tests passed!\n");
    return 0;
}
