#ifndef ANCIBLE_INVENTORY_H
#define ANCIBLE_INVENTORY_H

/**
 * Structure to hold a host in the inventory
 */
typedef struct host {
    char *name;           // Host name
    char *ansible_host;   // IP address or hostname
    struct host *next;    // Next host in the list
} host_t;

/**
 * Structure to hold a group of hosts
 */
typedef struct group {
    char *name;           // Group name
    host_t *hosts;        // Linked list of hosts in this group
    struct group *next;   // Next group in the list
} group_t;

/**
 * Structure to hold the inventory
 */
typedef struct {
    group_t *groups;      // Linked list of groups
    host_t *all_hosts;    // Linked list of all hosts
} inventory_t;

/**
 * Load inventory from a file
 * 
 * @param filename Path to the inventory file
 * @param inventory Pointer to inventory structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int inventory_load(const char *filename, inventory_t *inventory);

/**
 * Free resources used by an inventory
 * 
 * @param inventory Pointer to inventory structure to free
 */
void inventory_free(inventory_t *inventory);

/**
 * Get hosts in a group
 * 
 * @param inventory Pointer to inventory structure
 * @param group_name Name of the group to get hosts for
 * @return Pointer to the first host in the group, or NULL if group not found
 */
host_t *inventory_get_hosts(inventory_t *inventory, const char *group_name);

/**
 * Print inventory (for debugging)
 * 
 * @param inventory Pointer to inventory structure to print
 */
void inventory_print(const inventory_t *inventory);

#endif /* ANCIBLE_INVENTORY_H */
