#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include "../../include/ancible.h"
#include "../../include/core/state.h"
#include "../../include/modules/module.h"

/**
 * Check if a file exists
 */
static int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

/**
 * Test for state functionality
 */
int main(void) {
    printf("Running state tests\n");
    
    // Test 1: Initialize state
    {
        printf("Test 1: Initializing state... ");
        
        int ret = state_init();
        assert(ret == ANCIBLE_SUCCESS);
        
        // Check if runtime/state directory exists
        assert(file_exists("runtime/state"));
        
        printf("OK\n");
    }
    
    // Test 2: Save task result
    {
        printf("Test 2: Saving task result... ");
        
        module_result_t result;
        module_result_init(&result);
        
        result.changed = 1;
        result.failed = 0;
        result.msg = strdup("Test message");
        
        result.cmd_result.exit_code = 0;
        result.cmd_result.stdout_data = strdup("Test stdout");
        result.cmd_result.stderr_data = strdup("Test stderr");
        
        int ret = state_save_result("test_host", "test_task", &result);
        assert(ret == ANCIBLE_SUCCESS);
        
        // Check if result file exists
        assert(file_exists("runtime/state/test_host/last_run.json"));
        
        module_result_free(&result);
        
        printf("OK\n");
    }
    
    // Test 3: Clean up
    {
        printf("Test 3: Cleaning up state... ");
        
        state_cleanup();
        
        printf("OK\n");
    }
    
    printf("All state tests passed!\n");
    return 0;
}
