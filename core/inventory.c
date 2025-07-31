#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/ancible.h"
#include "../include/core/inventory.h"

#define MAX_LINE_LENGTH 1024

/**
 * Trim whitespace from a string (in-place)
 * 
 * @param str String to trim
 * @return Pointer to the trimmed string
 */
static char *trim(char *str) {
    if (!str) return NULL;
    
    // Trim leading space
    while(isspace((unsigned char)*str)) str++;
    
    if(*str == 0) return str;  // All spaces
    
    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    *(end + 1) = '\0';
    
    return str;
}

/**
 * Create a new host
 * 
 * @param name Host name
 * @return Pointer to the new host, or NULL on error
 */
static host_t *host_create(const char *name) {
    host_t *host = malloc(sizeof(host_t));
    if (!host) {
        fprintf(stderr, "Error: Failed to allocate memory for host\n");
        return NULL;
    }
    
    host->name = strdup(name);
    if (!host->name) {
        fprintf(stderr, "Error: Failed to allocate memory for host name\n");
        free(host);
        return NULL;
    }
    
    host->ansible_host = NULL;
    host->next = NULL;
    
    return host;
}

/**
 * Create a new group
 * 
 * @param name Group name
 * @return Pointer to the new group, or NULL on error
 */
static group_t *group_create(const char *name) {
    group_t *group = malloc(sizeof(group_t));
    if (!group) {
        fprintf(stderr, "Error: Failed to allocate memory for group\n");
        return NULL;
    }
    
    group->name = strdup(name);
    if (!group->name) {
        fprintf(stderr, "Error: Failed to allocate memory for group name\n");
        free(group);
        return NULL;
    }
    
    group->hosts = NULL;
    group->next = NULL;
    
    return group;
}

/**
 * Add a host to a group
 * 
 * @param group Pointer to the group
 * @param host Pointer to the host
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
static int group_add_host(group_t *group, host_t *host) {
    if (!group || !host) {
        return ANCIBLE_ERROR;
    }
    
    // Add host to the beginning of the list
    host->next = group->hosts;
    group->hosts = host;
    
    return ANCIBLE_SUCCESS;
}

/**
 * Find a group by name
 * 
 * @param inventory Pointer to the inventory
 * @param name Group name
 * @return Pointer to the group, or NULL if not found
 */
static group_t *inventory_find_group(inventory_t *inventory, const char *name) {
    if (!inventory || !name) {
        return NULL;
    }
    
    group_t *group = inventory->groups;
    while (group) {
        if (strcmp(group->name, name) == 0) {
            return group;
        }
        group = group->next;
    }
    
    return NULL;
}

/**
 * Find a host by name
 * 
 * @param inventory Pointer to the inventory
 * @param name Host name
 * @return Pointer to the host, or NULL if not found
 */
static host_t *inventory_find_host(inventory_t *inventory, const char *name) {
    if (!inventory || !name) {
        return NULL;
    }
    
    host_t *host = inventory->all_hosts;
    while (host) {
        if (strcmp(host->name, name) == 0) {
            return host;
        }
        host = host->next;
    }
    
    return NULL;
}

/**
 * Parse a host variable line (e.g., "web01 ansible_host=192.168.1.10 ansible_connection=local")
 * 
 * @param line Line to parse
 * @param host Pointer to the host to update
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
static int parse_host_vars(char *line, host_t *host) {
    if (!line || !host) {
        return ANCIBLE_ERROR;
    }
    
    // Find ansible_host variable
    char *var = strstr(line, "ansible_host=");
    if (var) {
        var += strlen("ansible_host=");
        
        // Find end of value (space or end of string)
        char *end = strchr(var, ' ');
        if (end) {
            *end = '\0';  // Temporarily terminate string
        }
        
        host->ansible_host = strdup(var);
        if (!host->ansible_host) {
            fprintf(stderr, "Error: Failed to allocate memory for ansible_host\n");
            return ANCIBLE_ERROR;
        }
        
        if (end) {
            *end = ' ';  // Restore space
        }
    }
    
    // Find ansible_connection variable (we don't store this in the host struct,
    // but we'll handle it in the context)
    
    return ANCIBLE_SUCCESS;
}

/**
 * Load inventory from a file
 * 
 * @param filename Path to the inventory file
 * @param inventory Pointer to inventory structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int inventory_load(const char *filename, inventory_t *inventory) {
    FILE *file = NULL;
    char line[MAX_LINE_LENGTH];
    int result = ANCIBLE_ERROR;
    group_t *current_group = NULL;
    
    // Initialize inventory structure
    memset(inventory, 0, sizeof(inventory_t));
    
    // Open file
    file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Failed to open inventory file: %s\n", filename);
        return ANCIBLE_ERROR;
    }
    
    // Create the "all" group
    group_t *all_group = group_create("all");
    if (!all_group) {
        goto cleanup;
    }
    
    // Add the "all" group to the inventory
    all_group->next = inventory->groups;
    inventory->groups = all_group;
    current_group = all_group;
    
    // Parse file line by line
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }
        
        // Skip empty lines and comments
        if (len == 0 || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Trim whitespace
        char *trimmed = trim(line);
        
        // Check for group header [group_name]
        if (trimmed[0] == '[' && trimmed[len-1] == ']') {
            // Extract group name
            trimmed[len-1] = '\0';
            char *group_name = trimmed + 1;
            
            // Create new group
            group_t *group = group_create(group_name);
            if (!group) {
                goto cleanup;
            }
            
            // Add group to inventory
            group->next = inventory->groups;
            inventory->groups = group;
            current_group = group;
        } else {
            // This is a host line
            char *host_name = trimmed;
            
            // Check for ansible_host variable
            char *space = strchr(host_name, ' ');
            if (space) {
                *space = '\0';
            }
            
            // Create or find host
            host_t *host = inventory_find_host(inventory, host_name);
            if (!host) {
                host = host_create(host_name);
                if (!host) {
                    goto cleanup;
                }
                
                // Add host to all_hosts list
                host->next = inventory->all_hosts;
                inventory->all_hosts = host;
                
                // Add host to current group
                if (current_group) {
                    if (group_add_host(current_group, host) != ANCIBLE_SUCCESS) {
                        goto cleanup;
                    }
                }
                
                // Also add to "all" group if not already there
                if (current_group != all_group) {
                    // Create a copy of the host for the "all" group
                    host_t *all_host = host_create(host_name);
                    if (!all_host) {
                        goto cleanup;
                    }
                    
                    if (group_add_host(all_group, all_host) != ANCIBLE_SUCCESS) {
                        free(all_host);
                        goto cleanup;
                    }
                }
            }
            
            // Parse host variables if present
            if (space) {
                if (parse_host_vars(space + 1, host) != ANCIBLE_SUCCESS) {
                    goto cleanup;
                }
            }
        }
    }
    
    result = ANCIBLE_SUCCESS;
    
cleanup:
    if (file) {
        fclose(file);
    }
    
    if (result != ANCIBLE_SUCCESS) {
        inventory_free(inventory);
    }
    
    return result;
}

/**
 * Free resources used by an inventory
 * 
 * @param inventory Pointer to inventory structure to free
 */
void inventory_free(inventory_t *inventory) {
    if (!inventory) {
        return;
    }
    
    // Free groups
    group_t *group = inventory->groups;
    while (group) {
        group_t *next_group = group->next;
        
        // Free hosts in group
        host_t *host = group->hosts;
        while (host) {
            host_t *next_host = host->next;
            free(host->name);
            free(host->ansible_host);
            free(host);
            host = next_host;
        }
        
        free(group->name);
        free(group);
        group = next_group;
    }
    
    // Free all_hosts (these are duplicates, so we've already freed the memory above)
    inventory->all_hosts = NULL;
    inventory->groups = NULL;
}

/**
 * Get hosts in a group
 * 
 * @param inventory Pointer to inventory structure
 * @param group_name Name of the group to get hosts for
 * @return Pointer to the first host in the group, or NULL if group not found
 */
host_t *inventory_get_hosts(inventory_t *inventory, const char *group_name) {
    if (!inventory || !group_name) {
        return NULL;
    }
    
    group_t *group = inventory_find_group(inventory, group_name);
    if (!group) {
        return NULL;
    }
    
    return group->hosts;
}

/**
 * Print inventory (for debugging)
 * 
 * @param inventory Pointer to inventory structure to print
 */
void inventory_print(const inventory_t *inventory) {
    if (!inventory) {
        return;
    }
    
    printf("Inventory:\n");
    
    group_t *group = inventory->groups;
    while (group) {
        printf("  Group: %s\n", group->name);
        
        host_t *host = group->hosts;
        while (host) {
            printf("    Host: %s", host->name);
            if (host->ansible_host) {
                printf(" (ansible_host=%s)", host->ansible_host);
            }
            printf("\n");
            host = host->next;
        }
        
        group = group->next;
    }
}
