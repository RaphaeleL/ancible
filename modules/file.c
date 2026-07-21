#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include "../include/ancible.h"
#include "../include/core/context.h"
#include "../include/transport/runner.h"
#include "../include/modules/module.h"
#include "../include/modules/file.h"

#define FILE_ARGS_MAX_PAIRS 16
#define FILE_ARGS_MAX_VALUE 512

typedef struct {
    char *path;
    char *state;
    char *mode;
} file_args_t;

/**
 * Parse a single key=value argument from a string.
 * Supports quoted values with simple unescaping.
 *
 * @param str Input string positioned at the start of a key
 * @param key Output buffer for the key
 * @param key_size Size of key buffer
 * @param value Output buffer for the value
 * @param value_size Size of value buffer
 * @return Pointer to the next argument position, or NULL when done
 */
static const char *parse_kv(const char *str, char *key, size_t key_size,
                            char *value, size_t value_size) {
    if (!str || !key || !value) {
        return NULL;
    }

    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    if (*str == '\0') {
        return NULL;
    }

    size_t i = 0;
    while (*str && *str != '=' && *str != ' ' && i < key_size - 1) {
        key[i++] = *str++;
    }
    key[i] = '\0';

    if (*str != '=') {
        value[0] = '\0';
        return (*str == '\0') ? NULL : str + 1;
    }
    str++;

    i = 0;
    char quote = '\0';
    if (*str == '\'' || *str == '"') {
        quote = *str++;
    }

    while (*str && i < value_size - 1) {
        if (quote && *str == quote) {
            str++;
            break;
        }
        if (!quote && isspace((unsigned char)*str)) {
            break;
        }
        if (*str == '\\' && quote && *(str + 1)) {
            str++;
        }
        value[i++] = *str++;
    }
    value[i] = '\0';

    while (*str && isspace((unsigned char)*str)) {
        str++;
    }

    return (*str == '\0') ? NULL : str;
}

/**
 * Parse file module arguments into a structured form.
 *
 * @param args Raw argument string
 * @param file_args Output structure to fill
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
static int file_args_parse(const char *args, file_args_t *file_args) {
    if (!file_args) {
        return ANCIBLE_ERROR;
    }

    memset(file_args, 0, sizeof(*file_args));

    if (!args || args[0] == '\0') {
        return ANCIBLE_SUCCESS;
    }

    const char *p = args;
    char key[64];
    char value[FILE_ARGS_MAX_VALUE];

    while (p) {
        p = parse_kv(p, key, sizeof(key), value, sizeof(value));

        if (key[0] == '\0') {
            continue;
        }

        if (strcmp(key, "path") == 0) {
            file_args->path = strdup(value);
        } else if (strcmp(key, "state") == 0) {
            file_args->state = strdup(value);
        } else if (strcmp(key, "mode") == 0) {
            file_args->mode = strdup(value);
        }
    }

    return ANCIBLE_SUCCESS;
}

/**
 * Free memory allocated by file_args_parse.
 */
static void file_args_free(file_args_t *file_args) {
    if (!file_args) {
        return;
    }

    free(file_args->path);
    free(file_args->state);
    free(file_args->mode);
    memset(file_args, 0, sizeof(*file_args));
}

/**
 * Convert a mode string to an octal mode value.
 *
 * @param mode_str Mode string (e.g. "0644" or "755")
 * @param mode Output mode value
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
static int parse_mode(const char *mode_str, mode_t *mode) {
    if (!mode_str || !mode) {
        return ANCIBLE_ERROR;
    }

    char *endptr = NULL;
    long value = strtol(mode_str, &endptr, 8);

    if (endptr == mode_str || *endptr != '\0' || value < 0 || value > 07777) {
        return ANCIBLE_ERROR;
    }

    *mode = (mode_t)value;
    return ANCIBLE_SUCCESS;
}

/**
 * Set a human-readable message on the result.
 */
static void set_msg(module_result_t *result, const char *fmt, ...) {
    if (!result || !fmt) {
        return;
    }

    free(result->msg);
    result->msg = malloc(512);
    if (!result->msg) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(result->msg, 512, fmt, args);
    va_end(args);
}

/**
 * Ensure a directory exists (mkdir -p style).
 *
 * @param path Directory path
 * @param mode Permissions to apply when creating
 * @param changed Output flag set to 1 if a directory was created
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
static int ensure_directory(const char *path, mode_t mode, int *changed) {
    struct stat st;

    if (stat(path, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            return ANCIBLE_ERROR;
        }
        *changed = 0;
        return ANCIBLE_SUCCESS;
    }

    if (errno != ENOENT) {
        return ANCIBLE_ERROR;
    }

    if (mkdir(path, mode) != 0) {
        return ANCIBLE_ERROR;
    }

    *changed = 1;
    return ANCIBLE_SUCCESS;
}

/**
 * Ensure a file exists.
 *
 * @param path File path
 * @param mode Permissions to apply when creating
 * @param touch Whether to update the timestamp of an existing file
 * @param changed Output flag set to 1 if a file was created or touched
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
static int ensure_file(const char *path, mode_t mode, int touch, int *changed) {
    struct stat st;

    if (stat(path, &st) == 0) {
        if (!S_ISREG(st.st_mode)) {
            return ANCIBLE_ERROR;
        }

        if (touch) {
            if (utimes(path, NULL) != 0) {
                return ANCIBLE_ERROR;
            }
            *changed = 1;
        } else {
            *changed = 0;
        }
        return ANCIBLE_SUCCESS;
    }

    if (errno != ENOENT) {
        return ANCIBLE_ERROR;
    }

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd < 0) {
        return ANCIBLE_ERROR;
    }
    close(fd);

    *changed = 1;
    return ANCIBLE_SUCCESS;
}

/**
 * Remove a file or directory.
 *
 * @param path Path to remove
 * @param changed Output flag set to 1 if something was removed
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
static int remove_path(const char *path, int *changed) {
    struct stat st;

    if (stat(path, &st) != 0) {
        if (errno == ENOENT) {
            *changed = 0;
            return ANCIBLE_SUCCESS;
        }
        return ANCIBLE_ERROR;
    }

    if (S_ISDIR(st.st_mode)) {
        if (rmdir(path) != 0) {
            return ANCIBLE_ERROR;
        }
    } else {
        if (unlink(path) != 0) {
            return ANCIBLE_ERROR;
        }
    }

    *changed = 1;
    return ANCIBLE_SUCCESS;
}

/**
 * Apply mode to an existing path if it differs.
 *
 * @param path Path to chmod
 * @param mode Desired mode
 * @param changed Output flag set to 1 if mode changed
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
static int apply_mode(const char *path, mode_t mode, int *changed) {
    struct stat st;

    if (stat(path, &st) != 0) {
        return ANCIBLE_ERROR;
    }

    if ((st.st_mode & 07777) == mode) {
        *changed = 0;
        return ANCIBLE_SUCCESS;
    }

    if (chmod(path, mode) != 0) {
        return ANCIBLE_ERROR;
    }

    *changed = 1;
    return ANCIBLE_SUCCESS;
}

int file_module_exec(context_t *context, const char *args, module_result_t *result) {
    if (!context || !result) {
        return ANCIBLE_ERROR;
    }

    module_result_init(result);

    file_args_t file_args;
    if (file_args_parse(args, &file_args) != ANCIBLE_SUCCESS) {
        result->failed = 1;
        set_msg(result, "Failed to parse file module arguments");
        return ANCIBLE_SUCCESS;
    }

    if (!file_args.path || file_args.path[0] == '\0') {
        result->failed = 1;
        set_msg(result, "No path specified");
        file_args_free(&file_args);
        return ANCIBLE_SUCCESS;
    }

    const char *state = file_args.state ? file_args.state : "present";
    mode_t mode = 0644;
    int has_mode = 0;

    if (file_args.mode) {
        if (parse_mode(file_args.mode, &mode) != ANCIBLE_SUCCESS) {
            result->failed = 1;
            set_msg(result, "Invalid mode: %s", file_args.mode);
            file_args_free(&file_args);
            return ANCIBLE_SUCCESS;
        }
        has_mode = 1;
    }

    int changed = 0;
    int rc = ANCIBLE_SUCCESS;

    if (strcmp(state, "absent") == 0) {
        rc = remove_path(file_args.path, &changed);
        if (rc != ANCIBLE_SUCCESS) {
            result->failed = 1;
            set_msg(result, "Failed to remove %s", file_args.path);
        } else {
            set_msg(result, changed ? "Removed %s" : "%s already absent", file_args.path);
        }
    } else if (strcmp(state, "directory") == 0) {
        rc = ensure_directory(file_args.path, mode, &changed);
        if (rc != ANCIBLE_SUCCESS) {
            result->failed = 1;
            set_msg(result, "Failed to create directory %s", file_args.path);
        } else if (has_mode) {
            int mode_changed = 0;
            rc = apply_mode(file_args.path, mode, &mode_changed);
            if (rc != ANCIBLE_SUCCESS) {
                result->failed = 1;
                set_msg(result, "Failed to set mode on %s", file_args.path);
            } else {
                changed = changed || mode_changed;
                set_msg(result, changed ? "Created or updated directory %s" : "Directory %s is in desired state", file_args.path);
            }
        } else {
            set_msg(result, changed ? "Created directory %s" : "Directory %s already exists", file_args.path);
        }
    } else if (strcmp(state, "touch") == 0) {
        rc = ensure_file(file_args.path, mode, 1, &changed);
        if (rc != ANCIBLE_SUCCESS) {
            result->failed = 1;
            set_msg(result, "Failed to touch %s", file_args.path);
        } else if (has_mode) {
            int mode_changed = 0;
            rc = apply_mode(file_args.path, mode, &mode_changed);
            if (rc != ANCIBLE_SUCCESS) {
                result->failed = 1;
                set_msg(result, "Failed to set mode on %s", file_args.path);
            } else {
                changed = changed || mode_changed;
                set_msg(result, changed ? "Touched or updated %s" : "File %s is in desired state", file_args.path);
            }
        } else {
            set_msg(result, changed ? "Touched %s" : "File %s already exists", file_args.path);
        }
    } else if (strcmp(state, "present") == 0 || strcmp(state, "file") == 0) {
        rc = ensure_file(file_args.path, mode, 0, &changed);
        if (rc != ANCIBLE_SUCCESS) {
            result->failed = 1;
            set_msg(result, "Failed to create %s", file_args.path);
        } else if (has_mode) {
            int mode_changed = 0;
            rc = apply_mode(file_args.path, mode, &mode_changed);
            if (rc != ANCIBLE_SUCCESS) {
                result->failed = 1;
                set_msg(result, "Failed to set mode on %s", file_args.path);
            } else {
                changed = changed || mode_changed;
                set_msg(result, changed ? "Created or updated %s" : "File %s is in desired state", file_args.path);
            }
        } else {
            set_msg(result, changed ? "Created %s" : "File %s already exists", file_args.path);
        }
    } else {
        result->failed = 1;
        set_msg(result, "Unknown state: %s", state);
        rc = ANCIBLE_SUCCESS;
    }

    result->changed = changed;
    file_args_free(&file_args);
    return rc;
}
