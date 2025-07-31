#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/ancible.h"
#include "../../include/core/parser.h"

/**
 * Test for parser.c functionality
 */
int main(void) {
    printf("Running parser.c tests\n");
    
    // Test 1: Parse simple playbook
    {
        printf("Test 1: Parsing simple playbook... ");
        playbook_t playbook;
        int result = parse_playbook("../../examples/playbooks/simple.yml", &playbook);
        
        assert(result == ANCIBLE_SUCCESS);
        assert(playbook.hosts != NULL);
        assert(strcmp(playbook.hosts, "all") == 0);
        assert(playbook.task_count == 1);
        assert(playbook.task_names != NULL);
        assert(playbook.task_modules != NULL);
        assert(strcmp(playbook.task_names[0], "Echo a message") == 0);
        assert(strcmp(playbook.task_modules[0], "command") == 0);
        
        playbook_free(&playbook);
        printf("OK\n");
    }
    
    // Test 2: Parse nonexistent playbook
    {
        printf("Test 2: Parsing nonexistent playbook... ");
        playbook_t playbook;
        int result = parse_playbook("../../examples/playbooks/nonexistent.yml", &playbook);
        
        assert(result == ANCIBLE_ERROR);
        printf("OK\n");
    }
    
    printf("All parser.c tests passed!\n");
    return 0;
}
