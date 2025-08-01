#ifndef ANCIBLE_ARGS_H
#define ANCIBLE_ARGS_H

/**
 * Structure to hold command-line options
 */
struct cli_options {
    int help;              // Whether --help was specified
    int verbose;           // Whether --verbose was specified
    int color;             // Whether color output is enabled (not used in this MVP)
    const char *playbook_path;  // Path to the playbook file
    const char *inventory_path; // Path to the inventory file
};

/**
 * Parse command-line arguments for ancible-playbook
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @param options Pointer to options structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int parse_args(int argc, char *argv[], struct cli_options *options);

#endif /* ANCIBLE_ARGS_H */
