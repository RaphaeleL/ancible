#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/ancible.h"
#include "../include/core/parser.h"

#define MAX_LINE_LENGTH 1024
#define MAX_TASKS 100
#define MAX_SUBTASKS 50

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
    int in_block = 0;
    int in_rescue = 0;
    int in_always = 0;
    int current_block = -1;
    int current_indent = 0;
    int block_indent = 0;
    
    // Initialize playbook structure
    memset(playbook, 0, sizeof(playbook_t));
    
    // Allocate memory for tasks
    playbook->tasks = malloc(MAX_TASKS * sizeof(task_t));
    if (!playbook->tasks) {
        fprintf(stderr, "Error: Failed to allocate memory for tasks\n");
        return ANCIBLE_ERROR;
    }
    
    // Initialize tasks
    for (int i = 0; i < MAX_TASKS; i++) {
        playbook->tasks[i].name = NULL;
        playbook->tasks[i].module = NULL;
        playbook->tasks[i].when = NULL;
        playbook->tasks[i].type = TASK_TYPE_NORMAL;
        playbook->tasks[i].parent_idx = -1;
        playbook->tasks[i].subtask_count = 0;
        playbook->tasks[i].subtask_indices = NULL;
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
        
        // Calculate indentation
        current_indent = 0;
        while (current_indent < (int)len && line[current_indent] == ' ') {
            current_indent++;
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
            // Check for block start
            if (strstr(line, "block:")) {
                // Convert the current task to a block (if it exists)
                if (current_task >= 0) {
                    playbook->tasks[current_task].type = TASK_TYPE_BLOCK;
                    playbook->tasks[current_task].subtask_indices = malloc(MAX_SUBTASKS * sizeof(int));
                    if (!playbook->tasks[current_task].subtask_indices) {
                        fprintf(stderr, "Error: Failed to allocate memory for subtasks\n");
                        goto cleanup;
                    }
                    
                    // Update state
                    current_block = current_task;
                    in_block = 1;
                    block_indent = current_indent;
                } else {
                    // Create a new block task if no current task
                    current_task++;
                    if (current_task >= MAX_TASKS) {
                        fprintf(stderr, "Error: Too many tasks (max %d)\n", MAX_TASKS);
                        goto cleanup;
                    }
                    
                    // Set block properties
                    playbook->tasks[current_task].type = TASK_TYPE_BLOCK;
                    playbook->tasks[current_task].subtask_indices = malloc(MAX_SUBTASKS * sizeof(int));
                    if (!playbook->tasks[current_task].subtask_indices) {
                        fprintf(stderr, "Error: Failed to allocate memory for subtasks\n");
                        goto cleanup;
                    }
                    
                    // Update state
                    current_block = current_task;
                    in_block = 1;
                    block_indent = current_indent;
                    
                    // Update task count
                    playbook->task_count = current_task + 1;
                }
                continue;
            }
            
            // Check for rescue block
            if (strstr(line, "rescue:")) {
                if (!in_block || current_block < 0) {
                    fprintf(stderr, "Error: 'rescue' outside of a block\n");
                    goto cleanup;
                }
                
                // Create a new task for the rescue block
                current_task++;
                if (current_task >= MAX_TASKS) {
                    fprintf(stderr, "Error: Too many tasks (max %d)\n", MAX_TASKS);
                    goto cleanup;
                }
                
                // Set rescue block properties
                playbook->tasks[current_task].type = TASK_TYPE_RESCUE;
                playbook->tasks[current_task].parent_idx = current_block;
                playbook->tasks[current_task].subtask_indices = malloc(MAX_SUBTASKS * sizeof(int));
                if (!playbook->tasks[current_task].subtask_indices) {
                    fprintf(stderr, "Error: Failed to allocate memory for subtasks\n");
                    goto cleanup;
                }
                
                // Update state
                in_block = 0;
                in_rescue = 1;
                
                // Update task count
                playbook->task_count = current_task + 1;
                continue;
            }
            
            // Check for always block
            if (strstr(line, "always:")) {
                if (!in_block && !in_rescue) {
                    fprintf(stderr, "Error: 'always' outside of a block or rescue\n");
                    goto cleanup;
                }
                
                // Create a new task for the always block
                current_task++;
                if (current_task >= MAX_TASKS) {
                    fprintf(stderr, "Error: Too many tasks (max %d)\n", MAX_TASKS);
                    goto cleanup;
                }
                
                // Set always block properties
                playbook->tasks[current_task].type = TASK_TYPE_ALWAYS;
                playbook->tasks[current_task].parent_idx = current_block;
                playbook->tasks[current_task].subtask_indices = malloc(MAX_SUBTASKS * sizeof(int));
                if (!playbook->tasks[current_task].subtask_indices) {
                    fprintf(stderr, "Error: Failed to allocate memory for subtasks\n");
                    goto cleanup;
                }
                
                // Update state
                in_block = 0;
                in_rescue = 0;
                in_always = 1;
                
                // Update task count
                playbook->task_count = current_task + 1;
                continue;
            }
            
            // Check for task name (handle "- name:" format)
            if (strstr(line, "name:")) {
                // Check if we're exiting a block based on indentation
                if ((in_block || in_rescue || in_always) && current_indent <= block_indent) {
                    in_block = 0;
                    in_rescue = 0;
                    in_always = 0;
                    current_block = -1;
                }
                
                // Create a new task
                current_task++;
                if (current_task >= MAX_TASKS) {
                    fprintf(stderr, "Error: Too many tasks (max %d)\n", MAX_TASKS);
                    goto cleanup;
                }
                
                // Extract task name
                char *name_start = strchr(line, ':') + 1;
                while (isspace(*name_start)) name_start++;
                
                playbook->tasks[current_task].name = strdup(name_start);
                if (!playbook->tasks[current_task].name) {
                    fprintf(stderr, "Error: Failed to allocate memory for task name\n");
                    goto cleanup;
                }
                
                // Set parent if inside a block
                if (in_block) {
                    playbook->tasks[current_task].parent_idx = current_block;
                    
                    // Add this task to the block's subtasks
                    int subtask_idx = playbook->tasks[current_block].subtask_count;
                    if (subtask_idx >= MAX_SUBTASKS) {
                        fprintf(stderr, "Error: Too many subtasks in block (max %d)\n", MAX_SUBTASKS);
                        goto cleanup;
                    }
                    
                    playbook->tasks[current_block].subtask_indices[subtask_idx] = current_task;
                    playbook->tasks[current_block].subtask_count++;
                } else if (in_rescue) {
                    playbook->tasks[current_task].parent_idx = current_task - 1; // Rescue block
                    
                    // Add this task to the rescue block's subtasks
                    int rescue_block = current_task - 1;
                    int subtask_idx = playbook->tasks[rescue_block].subtask_count;
                    if (subtask_idx >= MAX_SUBTASKS) {
                        fprintf(stderr, "Error: Too many subtasks in rescue block (max %d)\n", MAX_SUBTASKS);
                        goto cleanup;
                    }
                    
                    playbook->tasks[rescue_block].subtask_indices[subtask_idx] = current_task;
                    playbook->tasks[rescue_block].subtask_count++;
                } else if (in_always) {
                    playbook->tasks[current_task].parent_idx = current_task - 1; // Always block
                    
                    // Add this task to the always block's subtasks
                    int always_block = current_task - 1;
                    int subtask_idx = playbook->tasks[always_block].subtask_count;
                    if (subtask_idx >= MAX_SUBTASKS) {
                        fprintf(stderr, "Error: Too many subtasks in always block (max %d)\n", MAX_SUBTASKS);
                        goto cleanup;
                    }
                    
                    playbook->tasks[always_block].subtask_indices[subtask_idx] = current_task;
                    playbook->tasks[always_block].subtask_count++;
                }
                
                // Update task count
                playbook->task_count = current_task + 1;
            }
            // Check for module name (first indented key after a name)
            else if (current_task >= 0 && strchr(line, ':') && !playbook->tasks[current_task].module) {
                // Skip lines that are too indented (like cmd: inside command:)
                if (current_indent > 12) continue;
                
                // Skip if this is a block, rescue, or always
                if (strstr(line, "block:") || strstr(line, "rescue:") || strstr(line, "always:")) {
                    continue;
                }
                
                char *module_end = strchr(line, ':');
                *module_end = '\0';
                
                // Find the start of the module name (after spaces)
                char *module_start = line;
                while (isspace(*module_start)) module_start++;
                
                playbook->tasks[current_task].module = strdup(module_start);
                if (!playbook->tasks[current_task].module) {
                    fprintf(stderr, "Error: Failed to allocate memory for task module\n");
                    goto cleanup;
                }
            }
            // Check for when condition
            else if (current_task >= 0 && strstr(line, "when:") && !playbook->tasks[current_task].when) {
                // Extract when condition
                char *when_start = strchr(line, ':') + 1;
                while (isspace(*when_start)) when_start++;
                
                playbook->tasks[current_task].when = strdup(when_start);
                if (!playbook->tasks[current_task].when) {
                    fprintf(stderr, "Error: Failed to allocate memory for task when condition\n");
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
    
    if (playbook->tasks) {
        for (int i = 0; i < playbook->task_count; i++) {
            if (playbook->tasks[i].name) {
                free(playbook->tasks[i].name);
            }
            
            if (playbook->tasks[i].module) {
                free(playbook->tasks[i].module);
            }
            
            if (playbook->tasks[i].when) {
                free(playbook->tasks[i].when);
            }
            
            if (playbook->tasks[i].subtask_indices) {
                free(playbook->tasks[i].subtask_indices);
            }
        }
        
        free(playbook->tasks);
        playbook->tasks = NULL;
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
        
        // Print task type
        const char *type_str = "Normal";
        if (playbook->tasks[i].type == TASK_TYPE_BLOCK) {
            type_str = "Block";
        } else if (playbook->tasks[i].type == TASK_TYPE_RESCUE) {
            type_str = "Rescue";
        } else if (playbook->tasks[i].type == TASK_TYPE_ALWAYS) {
            type_str = "Always";
        }
        
        printf("      Type: %s\n", type_str);
        
        // Print task name if available
        if (playbook->tasks[i].name) {
            printf("      Name: %s\n", playbook->tasks[i].name);
        }
        
        // Print module if available
        if (playbook->tasks[i].module) {
            printf("      Module: %s\n", playbook->tasks[i].module);
        }
        
        // Print when condition if available
        if (playbook->tasks[i].when) {
            printf("      When: %s\n", playbook->tasks[i].when);
        }
        
        // Print parent index if not top-level
        if (playbook->tasks[i].parent_idx >= 0) {
            printf("      Parent: %d\n", playbook->tasks[i].parent_idx + 1);
        }
        
        // Print subtasks if any
        if (playbook->tasks[i].subtask_count > 0) {
            printf("      Subtasks (%d):", playbook->tasks[i].subtask_count);
            for (int j = 0; j < playbook->tasks[i].subtask_count; j++) {
                printf(" %d", playbook->tasks[i].subtask_indices[j] + 1);
            }
            printf("\n");
        }
    }
}
