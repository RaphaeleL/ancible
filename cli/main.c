#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/ancible.h"
#include "../include/cli/args.h"
#include "../include/core/parser.h"
#include "../include/core/inventory.h"
#include "../include/core/context.h"
#include "../include/core/executor.h"
#include "../include/core/state.h"
#include "../include/transport/runner.h"
#include "../include/modules/module.h"

/**
 * Print usage information for ancible-playbook
 */
void print_usage(const char *program_name) {
    printf("Usage: %s [options] playbook.yml\n\n", program_name);
    printf("Options:\n");
    printf("  --help        Display this help message and exit\n");
    printf("  -v, --verbose Increase verbosity\n");
    printf("  -i INVENTORY  Specify inventory file (default: ./inventory.ini)\n");
    printf("\n");
    printf("Ancible: High-performance, C-based implementation of Ansible\n");
}

/**
 * Main entry point for ancible-playbook
 */
int main(int argc, char *argv[]) {
    struct cli_options options;
    
    // Parse command-line arguments
    int result = parse_args(argc, argv, &options);
    
    // Handle help flag
    if (options.help) {
        print_usage(argv[0]);
        return 0;
    }
    
    // Handle parsing errors
    if (result != ANCIBLE_SUCCESS) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Display basic info
    printf("Ancible playbook runner (MVP)\n");
    if (options.verbose) {
        printf("Verbose mode enabled\n");
    }
    printf("Playbook: %s\n", options.playbook_path);
    
    // Parse playbook
    playbook_t playbook;
    result = parse_playbook(options.playbook_path, &playbook);
    if (result != ANCIBLE_SUCCESS) {
        fprintf(stderr, "Error parsing playbook\n");
        return 1;
    }
    
    // Initialize executor
    result = executor_init();
    if (result != ANCIBLE_SUCCESS) {
        fprintf(stderr, "Error initializing executor\n");
        playbook_free(&playbook);
        return 1;
    }
    
    // Initialize state
    result = state_init();
    if (result != ANCIBLE_SUCCESS) {
        fprintf(stderr, "Error initializing state\n");
        executor_cleanup();
        playbook_free(&playbook);
        return 1;
    }
    
    // Print playbook information
    printf("\nPlaybook details:\n");
    playbook_print(&playbook);
    
    // Load inventory
    inventory_t inventory;
    result = inventory_load(options.inventory_path, &inventory);
    if (result != ANCIBLE_SUCCESS) {
        fprintf(stderr, "Error loading inventory: %s\n", options.inventory_path);
        playbook_free(&playbook);
        return 1;
    }
    
    // Print inventory information
    printf("\nInventory details:\n");
    inventory_print(&inventory);
    
    // Get hosts for the playbook
    host_t *hosts = inventory_get_hosts(&inventory, playbook.hosts);
    if (!hosts) {
        fprintf(stderr, "Error: No hosts found for group '%s'\n", playbook.hosts);
        inventory_free(&inventory);
        playbook_free(&playbook);
        return 1;
    }
    
    printf("\nTargeted hosts:\n");
    host_t *host = hosts;
    while (host) {
        printf("  %s", host->name);
        if (host->ansible_host) {
            printf(" (ansible_host=%s)", host->ansible_host);
        }
        printf("\n");
        
        // Create context for this host
        context_t *context = context_create(host, &playbook, options.verbose);
        if (!context) {
            fprintf(stderr, "Error: Failed to create context for host %s\n", host->name);
            continue;
        }
        
        // Set some default variables
        context_set_var(context, "ansible_user", "root");
        context_set_var(context, "ansible_connection", "local");
        
        // Print context
        printf("\nContext for host %s:\n", host->name);
        context_print(context);
        
        // Run tasks for this host
        if (playbook.task_count > 0) {
            printf("\nRunning tasks for host %s:\n", host->name);
            
            for (int i = 0; i < playbook.task_count; i++) {
                printf("\nTASK [%s] *************\n", playbook.task_names[i]);
                
                // Extract command directly from the playbook file
                FILE *file = fopen(options.playbook_path, "r");
                char line[1024];
                char args[1024] = {0};
                
                if (file) {
                    // Find the task by name
                    int found_task = 0;
                    while (fgets(line, sizeof(line), file)) {
                        // Remove trailing newline
                        size_t len = strlen(line);
                        if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
                            line[--len] = '\0';
                        }
                        
                        // Look for the task name
                        if (!found_task && strstr(line, "name:") && strstr(line, playbook.task_names[i])) {
                            found_task = 1;
                            continue;
                        }
                        
                        // If we found the task, look for the module
                        if (found_task && strstr(line, playbook.task_modules[i])) {
                            char *cmd_start = strchr(line, ':');
                            if (cmd_start) {
                                cmd_start++; // Move past the colon
                                
                                // Skip leading whitespace
                                while (*cmd_start && isspace(*cmd_start)) {
                                    cmd_start++;
                                }
                                
                                // Copy the command
                                strncpy(args, cmd_start, sizeof(args) - 1);
                                args[sizeof(args) - 1] = '\0';
                                break;
                            }
                        }
                    }
                    
                    fclose(file);
                }
                
                // If we couldn't find the command, use a fallback
                if (args[0] == '\0') {
                    if (strcmp(playbook.task_modules[i], "command") == 0) {
                        snprintf(args, sizeof(args), "echo 'Command not found for task %s'", 
                                 playbook.task_names[i]);
                    } else {
                        snprintf(args, sizeof(args), "echo 'Unknown module %s'", 
                                 playbook.task_modules[i]);
                    }
                }
                
                // Execute task
                module_result_t result;
                if (executor_run_task(context, i, args, &result) == ANCIBLE_SUCCESS) {
                    printf("%s : %s\n", host->name, 
                           result.failed ? "FAILED" : (result.changed ? "CHANGED" : "SUCCESS"));
                    
                    if (result.msg) {
                        printf("  Message: %s\n", result.msg);
                    }
                    
                    if (result.cmd_result.stdout_data && strlen(result.cmd_result.stdout_data) > 0) {
                        printf("  Stdout: %s", result.cmd_result.stdout_data);
                    }
                    
                    if (result.cmd_result.stderr_data && strlen(result.cmd_result.stderr_data) > 0) {
                        printf("  Stderr: %s", result.cmd_result.stderr_data);
                    }
                    
                    // Save task result to state
                    state_save_result(host->name, playbook.task_names[i], &result);
                    
                    module_result_free(&result);
                } else {
                    printf("%s : ERROR\n", host->name);
                    printf("  Failed to execute task\n");
                }
            }
        }
        
        // Free context
        context_free(context);
        
        host = host->next;
    }
    
    // Clean up
    state_cleanup();
    executor_cleanup();
    inventory_free(&inventory);
    playbook_free(&playbook);
    
    return 0;
}
