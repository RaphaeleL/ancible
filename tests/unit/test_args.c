#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/ancible.h"
#include "../../include/cli/args.h"

/**
 * Test for args.c functionality
 */
int main(void) {
    printf("Running args.c tests\n");
    
    struct cli_options options;
    int result;
    
    // Test 1: Help flag
    {
        printf("Test 1: Testing help flag... ");
        char *argv[] = {"ancible-playbook", "--help"};
        result = parse_args(2, argv, &options);
        assert(result == ANCIBLE_SUCCESS);
        assert(options.help == 1);
        assert(strcmp(options.inventory_path, "inventory.ini") == 0);
        printf("OK\n");
    }
    
    // Test 2: Verbose flag
    {
        printf("Test 2: Testing verbose flag... ");
        char *argv[] = {"ancible-playbook", "-v", "test.yml"};
        
        // Create a test file
        FILE *fp = fopen("test.yml", "w");
        assert(fp != NULL);
        fprintf(fp, "# Test playbook\n");
        fclose(fp);
        
        result = parse_args(3, argv, &options);
        assert(result == ANCIBLE_SUCCESS);
        assert(options.verbose == 1);
        assert(strcmp(options.playbook_path, "test.yml") == 0);
        
        // Clean up
        remove("test.yml");
        printf("OK\n");
    }
    
    // Test 3: Missing playbook
    {
        printf("Test 3: Testing missing playbook... ");
        char *argv[] = {"ancible-playbook", "nonexistent.yml"};
        result = parse_args(2, argv, &options);
        assert(result == ANCIBLE_ERROR);
        printf("OK\n");
    }
    
    // Test 4: No playbook specified
    {
        printf("Test 4: Testing no playbook... ");
        char *argv[] = {"ancible-playbook", "-v"};
        result = parse_args(2, argv, &options);
        assert(result == ANCIBLE_ERROR);
        printf("OK\n");
    }
    
    // Test 5: Inventory flag
    {
        printf("Test 5: Testing inventory flag... ");
        char *argv[] = {"ancible-playbook", "-i", "custom_inventory.ini", "test.yml"};
        
        // Create a test file
        FILE *fp = fopen("test.yml", "w");
        assert(fp != NULL);
        fprintf(fp, "# Test playbook\n");
        fclose(fp);
        
        result = parse_args(4, argv, &options);
        assert(result == ANCIBLE_SUCCESS);
        assert(strcmp(options.inventory_path, "custom_inventory.ini") == 0);
        
        // Clean up
        remove("test.yml");
        printf("OK\n");
    }
    
    printf("All args.c tests passed!\n");
    return 0;
}
