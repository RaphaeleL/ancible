#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/ancible.h"
#include "../../include/transport/runner.h"

/**
 * Test for runner.c functionality
 */
int main(void) {
    printf("Running runner.c tests\n");
    
    // Test 1: Run echo command
    {
        printf("Test 1: Running echo command... ");
        
        command_result_t result;
        int ret = run_local("echo 'Hello, Ancible!'", &result);
        
        assert(ret == ANCIBLE_SUCCESS);
        assert(result.exit_code == 0);
        assert(result.stdout_data != NULL);
        assert(strstr(result.stdout_data, "Hello, Ancible!") != NULL);
        
        command_result_free(&result);
        printf("OK\n");
    }
    
    // Test 2: Run command with error
    {
        printf("Test 2: Running command with error... ");
        
        command_result_t result;
        int ret = run_local("ls /nonexistent_directory", &result);
        
        assert(ret == ANCIBLE_SUCCESS);
        assert(result.exit_code != 0);
        assert(result.stderr_data != NULL);
        assert(strlen(result.stderr_data) > 0);
        
        command_result_free(&result);
        printf("OK\n");
    }
    
    // Test 3: Run command with stdout and stderr
    {
        printf("Test 3: Running command with stdout and stderr... ");
        
        command_result_t result;
        int ret = run_local("echo 'stdout'; echo 'stderr' >&2", &result);
        
        assert(ret == ANCIBLE_SUCCESS);
        assert(result.exit_code == 0);
        assert(result.stdout_data != NULL);
        assert(strstr(result.stdout_data, "stdout") != NULL);
        assert(result.stderr_data != NULL);
        assert(strstr(result.stderr_data, "stderr") != NULL);
        
        command_result_free(&result);
        printf("OK\n");
    }
    
    printf("All runner.c tests passed!\n");
    return 0;
}
