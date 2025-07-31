#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/ancible.h"
#include "../../include/core/inventory.h"

/**
 * Test for inventory.c functionality
 */
int main(void) {
    printf("Running inventory.c tests\n");
    
    // Test 1: Load inventory file
    {
        printf("Test 1: Loading inventory file... ");
        inventory_t inventory;
        int result = inventory_load("../../examples/inventory.ini", &inventory);
        
        assert(result == ANCIBLE_SUCCESS);
        
        // Check that we have the expected groups
        group_t *group = inventory.groups;
        int group_count = 0;
        int found_webservers = 0;
        int found_dbservers = 0;
        int found_all = 0;
        
        while (group) {
            group_count++;
            if (strcmp(group->name, "webservers") == 0) {
                found_webservers = 1;
            } else if (strcmp(group->name, "dbservers") == 0) {
                found_dbservers = 1;
            } else if (strcmp(group->name, "all") == 0) {
                found_all = 1;
            }
            group = group->next;
        }
        
        assert(group_count >= 3); // at least webservers, dbservers, and all
        assert(found_webservers);
        assert(found_dbservers);
        assert(found_all);
        
        // Check that we can get hosts from a group
        host_t *webservers = inventory_get_hosts(&inventory, "webservers");
        assert(webservers != NULL);
        
        int web_host_count = 0;
        host_t *host = webservers;
        while (host) {
            web_host_count++;
            host = host->next;
        }
        assert(web_host_count == 2); // web01 and web02
        
        // Check that we can get hosts from the "all" group
        host_t *all_hosts = inventory_get_hosts(&inventory, "all");
        assert(all_hosts != NULL);
        
        inventory_free(&inventory);
        printf("OK\n");
    }
    
    // Test 2: Load nonexistent inventory file
    {
        printf("Test 2: Loading nonexistent inventory file... ");
        inventory_t inventory;
        int result = inventory_load("../../examples/nonexistent.ini", &inventory);
        
        assert(result == ANCIBLE_ERROR);
        printf("OK\n");
    }
    
    printf("All inventory.c tests passed!\n");
    return 0;
}
