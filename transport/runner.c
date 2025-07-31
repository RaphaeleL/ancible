#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../include/ancible.h"
#include "../include/transport/runner.h"
#include "../include/transport/ssh.h"

#define BUFFER_SIZE 4096

/**
 * Read all data from a file descriptor into a dynamically allocated string
 * 
 * @param fd File descriptor to read from
 * @return Dynamically allocated string, or NULL on error
 */
static char *read_all(int fd) {
    char buffer[BUFFER_SIZE];
    char *result = NULL;
    size_t result_len = 0;
    ssize_t bytes_read;
    
    // Create pipes to replace stdout and stderr
    FILE *stream = fdopen(fd, "r");
    if (!stream) {
        perror("fdopen");
        return NULL;
    }
    
    // Read all data from the stream
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, stream)) > 0) {
        // Resize result buffer
        char *new_result = realloc(result, result_len + bytes_read + 1);
        if (!new_result) {
            perror("realloc");
            free(result);
            fclose(stream);
            return NULL;
        }
        
        result = new_result;
        
        // Copy new data
        memcpy(result + result_len, buffer, bytes_read);
        result_len += bytes_read;
    }
    
    // Add null terminator
    if (result) {
        result[result_len] = '\0';
    } else {
        // Empty output
        result = strdup("");
        if (!result) {
            perror("strdup");
            fclose(stream);
            return NULL;
        }
    }
    
    fclose(stream);
    return result;
}

/**
 * Run a command locally
 * 
 * @param cmd Command to run
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int run_local(const char *cmd, command_result_t *result) {
    if (!cmd || !result) {
        return ANCIBLE_ERROR;
    }
    
    // Initialize result
    memset(result, 0, sizeof(command_result_t));
    
    // Create pipes for stdout and stderr
    int stdout_pipe[2];
    int stderr_pipe[2];
    
    if (pipe(stdout_pipe) == -1) {
        perror("pipe");
        return ANCIBLE_ERROR;
    }
    
    if (pipe(stderr_pipe) == -1) {
        perror("pipe");
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return ANCIBLE_ERROR;
    }
    
    // Fork a child process
    pid_t pid = fork();
    
    if (pid == -1) {
        // Fork failed
        perror("fork");
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        return ANCIBLE_ERROR;
    } else if (pid == 0) {
        // Child process
        
        // Close read ends of pipes
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        
        // Redirect stdout and stderr to pipes
        if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        
        if (dup2(stderr_pipe[1], STDERR_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        
        // Close write ends of pipes
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
        
        // Execute command
        execl("/bin/sh", "sh", "-c", cmd, NULL);
        
        // If execl returns, it failed
        perror("execl");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        
        // Close write ends of pipes
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
        
        // Read stdout and stderr
        result->stdout_data = read_all(stdout_pipe[0]);
        result->stderr_data = read_all(stderr_pipe[0]);
        
        // Close read ends of pipes
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        
        // Wait for child process to finish
        int status;
        waitpid(pid, &status, 0);
        
        // Get exit code
        if (WIFEXITED(status)) {
            result->exit_code = WEXITSTATUS(status);
        } else {
            result->exit_code = -1;
        }
        
        return ANCIBLE_SUCCESS;
    }
}

/**
 * Free resources used by a command result
 * 
 * @param result Pointer to result structure to free
 */
void command_result_free(command_result_t *result) {
    if (!result) {
        return;
    }
    
    free(result->stdout_data);
    free(result->stderr_data);
    
    result->stdout_data = NULL;
    result->stderr_data = NULL;
}

/**
 * Run a command on a host (local or remote)
 * 
 * @param context Execution context with host information
 * @param cmd Command to run
 * @param result Pointer to result structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
int run_command(context_t *context, const char *cmd, command_result_t *result) {
    if (!context || !cmd || !result) {
        return ANCIBLE_ERROR;
    }
    
    // Check connection type
    const char *connection = context_get_var(context, "ansible_connection");
    if (!connection) {
        connection = "ssh";  // Default connection type
    }
    
    // Execute command based on connection type
    if (strcmp(connection, "local") == 0) {
        // Run locally
        return run_local(cmd, result);
    } else if (strcmp(connection, "ssh") == 0) {
        // Run via SSH
        return run_ssh(context, cmd, result);
    } else {
        fprintf(stderr, "Error: Unsupported connection type: %s\n", connection);
        return ANCIBLE_ERROR;
    }
}

/**
 * Print command result (for debugging)
 * 
 * @param result Pointer to result structure to print
 */
void command_result_print(const command_result_t *result) {
    if (!result) {
        return;
    }
    
    printf("Command Result:\n");
    printf("  Exit Code: %d\n", result->exit_code);
    printf("  Stdout: %s\n", result->stdout_data ? result->stdout_data : "(null)");
    printf("  Stderr: %s\n", result->stderr_data ? result->stderr_data : "(null)");
}
