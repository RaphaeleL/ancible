#ifndef ANCIBLE_PARSER_H
#define ANCIBLE_PARSER_H

/**
 * Task type enumeration
 */
typedef enum {
    TASK_TYPE_NORMAL,    // Regular task
    TASK_TYPE_BLOCK,     // Block (contains other tasks)
    TASK_TYPE_RESCUE,    // Rescue block (error handling)
    TASK_TYPE_ALWAYS     // Always block (cleanup)
} task_type_t;

/**
 * Structure to hold task data
 */
typedef struct task {
    char *name;           // Task name
    char *module;         // Task module name
    char *when;           // Task when condition (may be NULL if no condition)
    task_type_t type;     // Task type
    int parent_idx;       // Index of parent block (-1 if top-level)
    int subtask_count;    // Number of subtasks (for blocks)
    int *subtask_indices; // Indices of subtasks (for blocks)
} task_t;

/**
 * Structure to hold playbook data
 */
typedef struct {
    char *hosts;          // Target hosts for this playbook
    int task_count;       // Number of tasks (including blocks and subtasks)
    task_t *tasks;        // Array of tasks
} playbook_t;

/**
 * Parse a YAML playbook file
 * 
 * @param filename Path to the YAML playbook file
 * @param playbook Pointer to playbook structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int parse_playbook(const char *filename, playbook_t *playbook);

/**
 * Free resources used by a playbook
 * 
 * @param playbook Pointer to playbook structure to free
 */
void playbook_free(playbook_t *playbook);

/**
 * Print playbook structure (for debugging)
 * 
 * @param playbook Pointer to playbook structure to print
 */
void playbook_print(const playbook_t *playbook);

#endif /* ANCIBLE_PARSER_H */
