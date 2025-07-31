#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/ancible.h"
#include "../include/cli/args.h"

/**
 * Parse command-line arguments for ancible-playbook
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @param options Pointer to options structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int parse_args(int argc, char *argv[], struct cli_options *options) {
    // Initialize options with defaults
    options->help = 0;
    options->verbose = 0;
    options->playbook_path = NULL;
    options->inventory_path = "inventory.ini"; // Default inventory path
    
    // No arguments provided
    if (argc < 2) {
        return ANCIBLE_ERROR;
    }
    
    // Process arguments
    for (int i = 1; i < argc; i++) {
        // Handle options (starting with - or --)
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                options->help = 1;
                return ANCIBLE_SUCCESS;
            } else if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
                options->verbose = 1;
            } else if (strcmp(argv[i], "-i") == 0) {
                // Check if there's a value after -i
                if (i + 1 >= argc) {
                    fprintf(stderr, "Error: -i requires an inventory file path\n");
                    return ANCIBLE_ERROR;
                }
                options->inventory_path = argv[++i];
            } else {
                fprintf(stderr, "Unknown option: %s\n", argv[i]);
                return ANCIBLE_ERROR;
            }
        } else {
            // Not an option, must be the playbook path
            if (options->playbook_path != NULL) {
                fprintf(stderr, "Error: Multiple playbook paths specified\n");
                return ANCIBLE_ERROR;
            }
            options->playbook_path = argv[i];
        }
    }
    
    // Validate that we have a playbook path
    if (options->playbook_path == NULL) {
        fprintf(stderr, "Error: No playbook path specified\n");
        return ANCIBLE_ERROR;
    }
    
    // Check if the playbook file exists
    if (access(options->playbook_path, F_OK) == -1) {
        fprintf(stderr, "Error: Playbook file not found: %s\n", options->playbook_path);
        return ANCIBLE_ERROR;
    }
    
    return ANCIBLE_SUCCESS;
}
