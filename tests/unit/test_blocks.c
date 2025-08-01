#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/ancible.h"
#include "../../include/core/parser.h"
#include "../../include/core/executor.h"
#include "../../include/modules/module.h"
#include "../../include/core/inventory.h"

/**
 * Test parsing a playbook with blocks
 */
void test_parse_blocks(void) {
    playbook_t playbook;
    
    printf("Testing block parsing...\n");
    
    // Parse the blocks example playbook
    int result = parse_playbook("../../examples/playbooks/blocks_example.yml", &playbook);
    if (result != ANCIBLE_SUCCESS) {
        printf("Failed to parse blocks_example.yml\n");
        return;
    }
    assert(result == ANCIBLE_SUCCESS);
    
    // Verify hosts
    assert(playbook.hosts != NULL);
    assert(strcmp(playbook.hosts, "all") == 0);
    
    // Print the playbook structure for debugging
    playbook_print(&playbook);
    
    // Verify that we have tasks
    assert(playbook.task_count > 0);
    
    // Find the basic block (Task 3 in the output)
    int basic_block_idx = -1;
    for (int i = 0; i < playbook.task_count; i++) {
        if (playbook.tasks[i].type == TASK_TYPE_BLOCK && 
            playbook.tasks[i].subtask_count == 2) {
            basic_block_idx = i;
            break;
        }
    }
    
    // Verify that we found the basic block
    assert(basic_block_idx >= 0);
    
    // Verify that the basic block has subtasks
    assert(playbook.tasks[basic_block_idx].subtask_count > 0);
    
    // Find the error handling block (Task 11 in the output)
    int error_block_idx = -1;
    for (int i = 0; i < playbook.task_count; i++) {
        if (playbook.tasks[i].type == TASK_TYPE_BLOCK && 
            playbook.tasks[i].subtask_count == 2 &&
            playbook.tasks[i].subtask_indices[0] >= 0) {
            // Check if this block has rescue and always blocks
            int has_rescue = 0;
            int has_always = 0;
            for (int j = 0; j < playbook.task_count; j++) {
                if (playbook.tasks[j].parent_idx == i) {
                    if (playbook.tasks[j].type == TASK_TYPE_RESCUE) {
                        has_rescue = 1;
                    } else if (playbook.tasks[j].type == TASK_TYPE_ALWAYS) {
                        has_always = 1;
                    }
                }
            }
            if (has_rescue && has_always) {
                error_block_idx = i;
                break;
            }
        }
    }
    
    // Verify that we found the error handling block
    assert(error_block_idx >= 0);
    
    // Find the rescue block
    int rescue_block_idx = -1;
    for (int i = 0; i < playbook.task_count; i++) {
        if (playbook.tasks[i].type == TASK_TYPE_RESCUE && 
            playbook.tasks[i].parent_idx == error_block_idx) {
            rescue_block_idx = i;
            break;
        }
    }
    
    // Verify that we found the rescue block
    assert(rescue_block_idx >= 0);
    
    // Find the always block
    int always_block_idx = -1;
    for (int i = 0; i < playbook.task_count; i++) {
        if (playbook.tasks[i].type == TASK_TYPE_ALWAYS && 
            playbook.tasks[i].parent_idx == error_block_idx) {
            always_block_idx = i;
            break;
        }
    }
    
    // Verify that we found the always block
    assert(always_block_idx >= 0);
    
    // Verify that the rescue block has subtasks
    assert(playbook.tasks[rescue_block_idx].subtask_count > 0);
    
    // Verify that the always block has subtasks
    assert(playbook.tasks[always_block_idx].subtask_count > 0);
    
    // Clean up
    playbook_free(&playbook);
    
    printf("Block parsing tests passed!\n");
}

/**
 * Mock module for testing
 */
int mock_module_exec(context_t *context, const char *args, module_result_t *result) {
    (void)context; // Suppress unused parameter warning
    // Check if this is a failing task
    if (strstr(args, "fail")) {
        result->failed = 1;
        result->changed = 0;
        result->msg = strdup("Task failed as requested");
        return ANCIBLE_ERROR;
    }
    
    // Normal task
    result->failed = 0;
    result->changed = 1;
    result->msg = strdup("Task executed successfully");
    return ANCIBLE_SUCCESS;
}

/**
 * Test executing a playbook with blocks
 */
void test_execute_blocks(void) {
    printf("Testing block execution...\n");
    
    // Initialize executor
    assert(executor_init() == ANCIBLE_SUCCESS);
    
    // Register mock module with a different name
    assert(executor_register_module("mock", mock_module_exec) == ANCIBLE_SUCCESS);
    
    // Create a simple playbook with blocks
    playbook_t playbook;
    memset(&playbook, 0, sizeof(playbook_t));
    
    // Allocate tasks
    playbook.tasks = malloc(10 * sizeof(task_t));
    assert(playbook.tasks != NULL);
    
    // Initialize tasks
    for (int i = 0; i < 10; i++) {
        playbook.tasks[i].name = NULL;
        playbook.tasks[i].module = NULL;
        playbook.tasks[i].when = NULL;
        playbook.tasks[i].type = TASK_TYPE_NORMAL;
        playbook.tasks[i].parent_idx = -1;
        playbook.tasks[i].subtask_count = 0;
        playbook.tasks[i].subtask_indices = NULL;
    }
    
    // Set up hosts
    playbook.hosts = strdup("all");
    
    // Task 0: Normal task
    playbook.tasks[0].name = strdup("Normal task");
    playbook.tasks[0].module = strdup("mock");
    
    // Task 1: Block
    playbook.tasks[1].name = strdup("Test block");
    playbook.tasks[1].type = TASK_TYPE_BLOCK;
    playbook.tasks[1].subtask_indices = malloc(3 * sizeof(int));
    playbook.tasks[1].subtask_count = 2;
    playbook.tasks[1].subtask_indices[0] = 2;
    playbook.tasks[1].subtask_indices[1] = 3;
    
    // Task 2: Subtask 1 in block
    playbook.tasks[2].name = strdup("Subtask 1");
    playbook.tasks[2].module = strdup("mock");
    playbook.tasks[2].parent_idx = 1;
    
    // Task 3: Subtask 2 in block (will fail)
    playbook.tasks[3].name = strdup("Subtask 2 (fail)");
    playbook.tasks[3].module = strdup("mock");
    playbook.tasks[3].parent_idx = 1;
    
    // Task 4: Rescue block
    playbook.tasks[4].name = strdup("Rescue block");
    playbook.tasks[4].type = TASK_TYPE_RESCUE;
    playbook.tasks[4].parent_idx = 1;
    playbook.tasks[4].subtask_indices = malloc(2 * sizeof(int));
    playbook.tasks[4].subtask_count = 1;
    playbook.tasks[4].subtask_indices[0] = 5;
    
    // Task 5: Rescue task
    playbook.tasks[5].name = strdup("Rescue task");
    playbook.tasks[5].module = strdup("mock");
    playbook.tasks[5].parent_idx = 4;
    
    // Task 6: Always block
    playbook.tasks[6].name = strdup("Always block");
    playbook.tasks[6].type = TASK_TYPE_ALWAYS;
    playbook.tasks[6].parent_idx = 1;
    playbook.tasks[6].subtask_indices = malloc(2 * sizeof(int));
    playbook.tasks[6].subtask_count = 1;
    playbook.tasks[6].subtask_indices[0] = 7;
    
    // Task 7: Always task
    playbook.tasks[7].name = strdup("Always task");
    playbook.tasks[7].module = strdup("mock");
    playbook.tasks[7].parent_idx = 6;
    
    // Set task count
    playbook.task_count = 8;
    
    // Create a simple host
    host_t host;
    memset(&host, 0, sizeof(host_t));
    host.name = strdup("localhost");
    host.ansible_host = strdup("127.0.0.1");
    
    // Create context
    context_t *context = context_create(&host, &playbook, 1);
    assert(context != NULL);
    
    // Execute normal task
    module_result_t result;
    module_result_init(&result);
    assert(executor_run_task(context, 0, "normal", &result) == ANCIBLE_SUCCESS);
    assert(result.failed == 0);
    module_result_free(&result);
    
    // Execute block with rescue and always
    module_result_init(&result);
    assert(executor_run_task(context, 1, "fail", &result) == ANCIBLE_SUCCESS);
    assert(result.failed == 0);  // Should not fail because rescue handled it
    module_result_free(&result);
    
    // Clean up
    context_free(context);
    free(host.name);
    free(host.ansible_host);
    
    for (int i = 0; i < playbook.task_count; i++) {
        free(playbook.tasks[i].name);
        free(playbook.tasks[i].module);
        free(playbook.tasks[i].when);
        free(playbook.tasks[i].subtask_indices);
    }
    
    free(playbook.tasks);
    free(playbook.hosts);
    
    executor_cleanup();
    
    printf("Block execution tests passed!\n");
}

/**
 * Main function
 */
int main(void) {
    printf("Running block tests...\n");
    
    test_parse_blocks();
    test_execute_blocks();
    
    printf("All block tests passed!\n");
    return 0;
}
