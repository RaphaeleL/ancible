#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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
    printf("  -c, --color   Enable colored output\n");
    printf("  -i INVENTORY  Specify inventory file (default: ./inventory.ini)\n");
    printf("\n");
    printf("Ancible: High-performance, C-based implementation of Ansible\n");
}

/**
 * Print colored messages to stdout
 * 
 * This function always prints, regardless of verbose mode
 * If color is enabled, it will colorize the output based on status
 */
void acout(struct cli_options options, module_result_t result, const char *fmt, ...) {
    const char *green = "\033[1;32m";
    const char *yellow = "\033[1;33m";
    const char *red = "\033[1;31m";
    const char *reset = "\033[0m";

    const char *no_color = "";

    if (result.changed) {
        printf("%s[CHANGED] %s", options.color ? green : no_color, reset);
    } else if (result.skipped) {
        printf("%s[SKIPPED] %s", options.color ? yellow : no_color, reset);
    } else {
        printf("%s[OK] %s", options.color ? red : no_color, reset);
    }

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    if (options.color) {
        printf("\033[0m");  // Reset color
    }
}

/**
 * Print messages to stdout if verbose mode is enabled
 */
void cout(int verbose, const char *fmt, ...) {
    if (verbose) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}

/**
 * Main entry point for ancible-playbook
 */
int main(int argc, char *argv[]) {
    // Initialize command-line options
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
    cout(options.verbose, "Ancible playbook runner (MVP)\n");
    if (options.verbose) {
        printf("Verbose mode enabled\n");
    }
    cout(options.verbose, "Playbook: %s\n", options.playbook_path);
    
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
    cout(options.verbose, "\nPlaybook details:\n");
    if (options.verbose) {
        playbook_print(&playbook);
    }
    
    // Load inventory
    inventory_t inventory;
    result = inventory_load(options.inventory_path, &inventory);
    if (result != ANCIBLE_SUCCESS) {
        fprintf(stderr, "Error loading inventory: %s\n", options.inventory_path);
        playbook_free(&playbook);
        return 1;
    }
    
    // Print inventory information
    cout(options.verbose, "\nInventory details:\n");
    if (options.verbose) {
        inventory_print(&inventory);
    }
    
    // Get hosts for the playbook
    host_t *hosts = inventory_get_hosts(&inventory, playbook.hosts);
    if (!hosts) {
        fprintf(stderr, "Error: No hosts found for group '%s'\n", playbook.hosts);
        inventory_free(&inventory);
        playbook_free(&playbook);
        return 1;
    }
    
    cout(options.verbose, "\nTargeted hosts:\n");
    host_t *host = hosts;
    while (host) {
        cout(options.verbose, "  %s", host->name);
        if (host->ansible_host) {
            cout(options.verbose, " (ansible_host=%s)", host->ansible_host);
        }
        cout(options.verbose, "\n");
        
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
        cout(options.verbose, "\nContext for host %s:\n", host->name);
        if (options.verbose) {
            context_print(context);
        }
        
        // Run tasks for this host
        if (playbook.task_count > 0) {
            cout(options.verbose, "\nRunning tasks for host %s:\n", host->name);
            
            for (int i = 0; i < playbook.task_count; i++) {
                cout(options.verbose, "\nTASK [%s] *************\n", playbook.task_names[i]);
                
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
                    acout(options, result, "%s\n", playbook.task_names[i]);
                    
                    if (result.msg) {
                        cout(options.verbose, "  Message: %s\n", result.msg);
                    }
                    
                    if (result.cmd_result.stdout_data && strlen(result.cmd_result.stdout_data) > 0) {
                        cout(options.verbose, "  Stdout: %s", result.cmd_result.stdout_data);
                    }
                    
                    if (result.cmd_result.stderr_data && strlen(result.cmd_result.stderr_data) > 0) {
                        cout(options.verbose, "  Stderr: %s", result.cmd_result.stderr_data);
                    }
                    
                    // Save task result to state
                    state_save_result(host->name, playbook.task_names[i], &result);
                    
                    module_result_free(&result);
                } else {
                    acout(options, result, "[ERROR] %s\n", playbook.task_names[i]);
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
