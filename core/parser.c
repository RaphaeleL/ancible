#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/ancible.h"
#include "../include/core/parser.h"

#define MAX_LINE_LENGTH 1024
#define MAX_TASKS 100

/**
 * Simple YAML parser for playbooks
 * 
 * This is a very simplified parser that only extracts hosts and task names/modules
 * from a playbook. It does not handle complex YAML structures.
 */

/**
 * Parse a YAML playbook file
 * 
 * @param filename Path to the YAML playbook file
 * @param playbook Pointer to playbook structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int parse_playbook(const char *filename, playbook_t *playbook) {
    FILE *file = NULL;
    char line[MAX_LINE_LENGTH];
    int result = ANCIBLE_ERROR;
    int in_tasks = 0;
    int current_task = -1;
    
    // Initialize playbook structure
    memset(playbook, 0, sizeof(playbook_t));
    
    // Allocate memory for task arrays
    playbook->task_names = malloc(MAX_TASKS * sizeof(char *));
    playbook->task_modules = malloc(MAX_TASKS * sizeof(char *));
    if (!playbook->task_names || !playbook->task_modules) {
        fprintf(stderr, "Error: Failed to allocate memory for tasks\n");
        return ANCIBLE_ERROR;
    }
    
    // Open file
    file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Failed to open file: %s\n", filename);
        return ANCIBLE_ERROR;
    }
    
    // Parse file line by line
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }
        
        // Skip empty lines and comments
        if (len == 0 || line[0] == '#' || (line[0] == ' ' && line[1] == '#')) {
            continue;
        }
        
        // Extract hosts (handle both "hosts: all" and "- hosts: all" formats)
        if (strstr(line, "hosts:")) {
            char *hosts_start = strchr(line, ':') + 1;
            while (isspace(*hosts_start)) hosts_start++;
            
            playbook->hosts = strdup(hosts_start);
            if (!playbook->hosts) {
                fprintf(stderr, "Error: Failed to allocate memory for hosts\n");
                goto cleanup;
            }
        }
        
        // Check if we're in the tasks section
        if (strstr(line, "tasks:")) {
            in_tasks = 1;
            continue;
        }
        
        // Parse tasks
        if (in_tasks) {
            // Check for task name (handle "- name:" format)
            if (strstr(line, "name:")) {
                current_task++;
                if (current_task >= MAX_TASKS) {
                    fprintf(stderr, "Error: Too many tasks (max %d)\n", MAX_TASKS);
                    goto cleanup;
                }
                
                char *name_start = strchr(line, ':') + 1;
                while (isspace(*name_start)) name_start++;
                
                playbook->task_names[current_task] = strdup(name_start);
                if (!playbook->task_names[current_task]) {
                    fprintf(stderr, "Error: Failed to allocate memory for task name\n");
                    goto cleanup;
                }
                
                playbook->task_count = current_task + 1;
            }
            // Check for module name (first indented key after a name)
            else if (current_task >= 0 && strchr(line, ':') && !playbook->task_modules[current_task]) {
                // Skip lines that are too indented (like cmd: inside command:)
                int indent = 0;
                while (line[indent] == ' ') indent++;
                if (indent > 8) continue;
                
                char *module_end = strchr(line, ':');
                *module_end = '\0';
                
                // Find the start of the module name (after spaces)
                char *module_start = line;
                while (isspace(*module_start)) module_start++;
                
                playbook->task_modules[current_task] = strdup(module_start);
                if (!playbook->task_modules[current_task]) {
                    fprintf(stderr, "Error: Failed to allocate memory for task module\n");
                    goto cleanup;
                }
            }
        }
    }
    
    // Validate that we found hosts and tasks
    if (!playbook->hosts) {
        fprintf(stderr, "Error: No hosts specified in playbook\n");
        goto cleanup;
    }
    
    if (playbook->task_count == 0) {
        fprintf(stderr, "Error: No tasks found in playbook\n");
        goto cleanup;
    }
    
    result = ANCIBLE_SUCCESS;
    
cleanup:
    if (file) {
        fclose(file);
    }
    
    if (result != ANCIBLE_SUCCESS) {
        playbook_free(playbook);
    }
    
    return result;
}

/**
 * Free resources used by a playbook
 * 
 * @param playbook Pointer to playbook structure to free
 */
void playbook_free(playbook_t *playbook) {
    if (!playbook) {
        return;
    }
    
    if (playbook->hosts) {
        free(playbook->hosts);
        playbook->hosts = NULL;
    }
    
    if (playbook->task_names) {
        for (int i = 0; i < playbook->task_count; i++) {
            if (playbook->task_names[i]) {
                free(playbook->task_names[i]);
            }
        }
        free(playbook->task_names);
        playbook->task_names = NULL;
    }
    
    if (playbook->task_modules) {
        for (int i = 0; i < playbook->task_count; i++) {
            if (playbook->task_modules[i]) {
                free(playbook->task_modules[i]);
            }
        }
        free(playbook->task_modules);
        playbook->task_modules = NULL;
    }
    
    playbook->task_count = 0;
}

/**
 * Print playbook structure (for debugging)
 * 
 * @param playbook Pointer to playbook structure to print
 */
void playbook_print(const playbook_t *playbook) {
    if (!playbook) {
        return;
    }
    
    printf("Playbook:\n");
    printf("  Hosts: %s\n", playbook->hosts ? playbook->hosts : "NULL");
    printf("  Tasks: %d\n", playbook->task_count);
    
    for (int i = 0; i < playbook->task_count; i++) {
        printf("    Task %d:\n", i + 1);
        printf("      Name: %s\n", playbook->task_names[i]);
        printf("      Module: %s\n", playbook->task_modules[i]);
    }
}
