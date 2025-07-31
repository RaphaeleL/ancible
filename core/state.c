#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "../include/ancible.h"
#include "../include/core/state.h"

#define STATE_DIR "runtime/state"

/**
 * Create a directory if it doesn't exist
 * 
 * @param path Directory path
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
static int create_directory(const char *path) {
    struct stat st;
    
    // Check if directory already exists
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return ANCIBLE_SUCCESS;
        }
        
        fprintf(stderr, "Error: %s exists but is not a directory\n", path);
        return ANCIBLE_ERROR;
    }
    
    // Create directory
#ifdef _WIN32
    if (mkdir(path) != 0) {
#else
    if (mkdir(path, 0755) != 0) {
#endif
        fprintf(stderr, "Error: Failed to create directory %s: %s\n", path, strerror(errno));
        return ANCIBLE_ERROR;
    }
    
    return ANCIBLE_SUCCESS;
}

/**
 * Initialize the state system
 * 
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int state_init(void) {
    // Create state directory
    if (create_directory(STATE_DIR) != ANCIBLE_SUCCESS) {
        return ANCIBLE_ERROR;
    }
    
    return ANCIBLE_SUCCESS;
}

/**
 * Save task result to state file
 * 
 * @param host_name Host name
 * @param task_name Task name
 * @param result Module result
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int state_save_result(const char *host_name, const char *task_name, const module_result_t *result) {
    if (!host_name || !task_name || !result) {
        return ANCIBLE_ERROR;
    }
    
    // Create host directory
    char host_dir[256];
    snprintf(host_dir, sizeof(host_dir), "%s/%s", STATE_DIR, host_name);
    
    if (create_directory(host_dir) != ANCIBLE_SUCCESS) {
        return ANCIBLE_ERROR;
    }
    
    // Create result file
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/last_run.json", host_dir);
    
    FILE *file = fopen(file_path, "w");
    if (!file) {
        fprintf(stderr, "Error: Failed to open %s for writing: %s\n", file_path, strerror(errno));
        return ANCIBLE_ERROR;
    }
    
    // Write JSON
    fprintf(file, "{\n");
    fprintf(file, "  \"task\": \"%s\",\n", task_name);
    fprintf(file, "  \"changed\": %s,\n", result->changed ? "true" : "false");
    fprintf(file, "  \"failed\": %s,\n", result->failed ? "true" : "false");
    
    if (result->msg) {
        fprintf(file, "  \"msg\": \"%s\",\n", result->msg);
    }
    
    fprintf(file, "  \"command\": {\n");
    fprintf(file, "    \"exit_code\": %d", result->cmd_result.exit_code);
    
    if (result->cmd_result.stdout_data && strlen(result->cmd_result.stdout_data) > 0) {
        // Remove newlines from stdout for JSON
        char *stdout_copy = strdup(result->cmd_result.stdout_data);
        if (stdout_copy) {
            char *p = stdout_copy;
            while (*p) {
                if (*p == '\n' || *p == '\r') {
                    *p = ' ';
                }
                p++;
            }
            fprintf(file, ",\n    \"stdout\": \"%s\"", stdout_copy);
            free(stdout_copy);
        }
    }
    
    if (result->cmd_result.stderr_data && strlen(result->cmd_result.stderr_data) > 0) {
        // Remove newlines from stderr for JSON
        char *stderr_copy = strdup(result->cmd_result.stderr_data);
        if (stderr_copy) {
            char *p = stderr_copy;
            while (*p) {
                if (*p == '\n' || *p == '\r') {
                    *p = ' ';
                }
                p++;
            }
            fprintf(file, ",\n    \"stderr\": \"%s\"", stderr_copy);
            free(stderr_copy);
        }
    }
    
    fprintf(file, "\n  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return ANCIBLE_SUCCESS;
}

/**
 * Clean up the state system
 */
void state_cleanup(void) {
    // Nothing to clean up for now
}
