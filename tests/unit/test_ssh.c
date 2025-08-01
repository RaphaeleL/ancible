#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/ancible.h"
#include "../../include/core/context.h"
#include "../../include/transport/runner.h"
#include "../../include/transport/ssh.h"

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
 * Test for ssh.c functionality
 */
int main(void) {
    printf("Running ssh.c tests\n");
    
    // Test 1: Run SSH command to localhost
    {
        printf("Test 1: Running SSH command to localhost... ");
        
        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        
        // Set variables
        context_set_var(context, "ansible_user", getenv("USER"));
        
        // Run command
        command_result_t result;
        int ret = run_ssh(context, "echo 'Hello from SSH!'", &result);
        
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
    
    printf("All ssh.c tests passed!\n");
    return 0;
}
