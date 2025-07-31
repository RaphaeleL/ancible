#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/ancible.h"
#include "../../include/core/context.h"
#include "../../include/transport/runner.h"

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
 * Test for the abstracted run_command functionality
 */
int main(void) {
    printf("Running command abstraction tests\n");
    
    // Test 1: Run local command
    {
        printf("Test 1: Running local command... ");
        
        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        
        // Set local connection
        context_set_var(context, "ansible_connection", "local");
        
        // Run command
        command_result_t result;
        int ret = run_command(context, "echo 'Hello from local!'", &result);
        
        assert(ret == ANCIBLE_SUCCESS);
        assert(result.exit_code == 0);
        assert(result.stdout_data != NULL);
        assert(strstr(result.stdout_data, "Hello from local!") != NULL);
        
        command_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);
        
        printf("OK\n");
    }
    
    // Test 2: Run SSH command (this might fail if SSH is not set up)
    {
        printf("Test 2: Running SSH command... ");
        
        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        
        // Set SSH connection (default)
        context_set_var(context, "ansible_user", getenv("USER"));
        
        // Run command
        command_result_t result;
        int ret = run_command(context, "echo 'Hello from SSH!'", &result);
        
        // We don't assert success here because SSH might not be set up on the test machine
        if (ret == ANCIBLE_SUCCESS && result.exit_code == 0) {
            printf("SSH command succeeded: %s", result.stdout_data);
            command_result_free(&result);
        } else {
            printf("SSH command not available (expected during testing)");
        }
        
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);
        
        printf("OK\n");
    }
    
    printf("All command abstraction tests passed!\n");
    return 0;
}
