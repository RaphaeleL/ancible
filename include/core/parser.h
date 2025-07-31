#ifndef ANCIBLE_PARSER_H
#define ANCIBLE_PARSER_H

/**
 * Structure to hold playbook data
 */
typedef struct {
    char *hosts;          // Target hosts for this playbook
    int task_count;       // Number of tasks
    char **task_names;    // Array of task names
    char **task_modules;  // Array of task module names
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
