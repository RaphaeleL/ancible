#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * Test for CLI functionality
 */
int main(void) {
    printf("Running CLI tests\n");
    
    // Test 1: Verify --help flag works
    printf("Test 1: Testing --help flag... ");
    // We'll use system() to run the actual binary and check its return code
    int result = system("../../bin/ancible-playbook --help > /dev/null");
    assert(WEXITSTATUS(result) == 0);
    printf("OK\n");
    
    // Test 2: Verify no arguments returns error
    printf("Test 2: Testing no arguments... ");
    result = system("../../bin/ancible-playbook > /dev/null");
    assert(WEXITSTATUS(result) == 1);
    printf("OK\n");
    
    // Test 3: Verify with playbook argument
    printf("Test 3: Testing with playbook argument... ");
    // Create a test playbook file
    FILE *fp = fopen("test.yml", "w");
    assert(fp != NULL);
    fprintf(fp, "---\n- hosts: all\n  tasks:\n    - name: Test task\n      command:\n        cmd: echo test\n");
    fclose(fp);
    
    // Create a test inventory file
    fp = fopen("inventory.ini", "w");
    assert(fp != NULL);
    fprintf(fp, "[all]\nlocalhost ansible_host=127.0.0.1\n");
    fclose(fp);
    
    // Create runtime/state directory
    system("mkdir -p runtime/state");
    
    result = system("../../bin/ancible-playbook test.yml > /dev/null");
    assert(WEXITSTATUS(result) == 0);
    
    // Clean up
    system("rm test.yml inventory.ini");
    printf("OK\n");
    
    printf("All CLI tests passed!\n");
    return 0;
}
