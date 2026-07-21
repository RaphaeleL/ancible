#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <libgen.h>
#include "../include/ancible.h"
#include "../include/core/context.h"
#include "../include/modules/module.h"
#include "../include/modules/copy.h"

#define COPY_ARGS_MAX_VALUE 4096

typedef struct {
    char *src;
    char *dest;
    char *content;
    char *mode;
    char *force;
} copy_args_t;

/**
 * Parse a single key=value argument from a string.
 * Supports quoted values with simple unescaping.
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
 * Parse copy module arguments into a structured form.
 */
static int copy_args_parse(const char *args, copy_args_t *copy_args) {
    if (!copy_args) {
        return ANCIBLE_ERROR;
    }

    memset(copy_args, 0, sizeof(*copy_args));

    if (!args || args[0] == '\0') {
        return ANCIBLE_SUCCESS;
    }

    const char *p = args;
    char key[64];
    char value[COPY_ARGS_MAX_VALUE];

    while (p) {
        p = parse_kv(p, key, sizeof(key), value, sizeof(value));

        if (key[0] == '\0') {
            continue;
        }

        if (strcmp(key, "src") == 0) {
            copy_args->src = strdup(value);
        } else if (strcmp(key, "dest") == 0) {
            copy_args->dest = strdup(value);
        } else if (strcmp(key, "content") == 0) {
            copy_args->content = strdup(value);
        } else if (strcmp(key, "mode") == 0) {
            copy_args->mode = strdup(value);
        } else if (strcmp(key, "force") == 0) {
            copy_args->force = strdup(value);
        }
    }

    return ANCIBLE_SUCCESS;
}

/**
 * Free memory allocated by copy_args_parse.
 */
static void copy_args_free(copy_args_t *copy_args) {
    if (!copy_args) {
        return;
    }

    free(copy_args->src);
    free(copy_args->dest);
    free(copy_args->content);
    free(copy_args->mode);
    free(copy_args->force);
    memset(copy_args, 0, sizeof(*copy_args));
}

/**
 * Convert a mode string to an octal mode value.
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
 * Parse a yes/no force flag. Defaults to yes (1).
 */
static int parse_force(const char *force_str) {
    if (!force_str || force_str[0] == '\0') {
        return 1;
    }

    if (strcmp(force_str, "no") == 0 || strcmp(force_str, "false") == 0 ||
        strcmp(force_str, "0") == 0) {
        return 0;
    }

    return 1;
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
 * Read an entire file into a newly allocated buffer.
 *
 * @param path File path
 * @param data Output pointer for allocated content
 * @param size Output size in bytes (not including null terminator)
 * @return ANCIBLE_SUCCESS on success, ANCIBLE_ERROR on error
 */
static int read_file(const char *path, char **data, size_t *size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return ANCIBLE_ERROR;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return ANCIBLE_ERROR;
    }

    long file_size = ftell(fp);
    if (file_size < 0) {
        fclose(fp);
        return ANCIBLE_ERROR;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return ANCIBLE_ERROR;
    }

    char *buf = malloc((size_t)file_size + 1);
    if (!buf) {
        fclose(fp);
        return ANCIBLE_ERROR;
    }

    size_t read_total = 0;
    while (read_total < (size_t)file_size) {
        size_t n = fread(buf + read_total, 1, (size_t)file_size - read_total, fp);
        if (n == 0) {
            if (ferror(fp)) {
                free(buf);
                fclose(fp);
                return ANCIBLE_ERROR;
            }
            break;
        }
        read_total += n;
    }

    buf[read_total] = '\0';
    fclose(fp);

    *data = buf;
    *size = read_total;
    return ANCIBLE_SUCCESS;
}

/**
 * Resolve destination path. If dest is an existing directory and src is set,
 * append the basename of src (Ansible-compatible behavior).
 */
static char *resolve_dest(const char *dest, const char *src) {
    struct stat st;

    if (stat(dest, &st) == 0 && S_ISDIR(st.st_mode) && src) {
        char src_copy[COPY_ARGS_MAX_VALUE];
        strncpy(src_copy, src, sizeof(src_copy) - 1);
        src_copy[sizeof(src_copy) - 1] = '\0';

        char *base = basename(src_copy);
        size_t needed = strlen(dest) + 1 + strlen(base) + 1;
        char *resolved = malloc(needed);
        if (!resolved) {
            return NULL;
        }

        if (dest[strlen(dest) - 1] == '/') {
            snprintf(resolved, needed, "%s%s", dest, base);
        } else {
            snprintf(resolved, needed, "%s/%s", dest, base);
        }
        return resolved;
    }

    return strdup(dest);
}

/**
 * Apply mode to an existing path if it differs.
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

/**
 * Write data to dest atomically via a temp file in the same directory.
 */
static int write_file(const char *dest, const char *data, size_t size, mode_t mode) {
    char dest_copy[COPY_ARGS_MAX_VALUE];
    strncpy(dest_copy, dest, sizeof(dest_copy) - 1);
    dest_copy[sizeof(dest_copy) - 1] = '\0';

    char *dir = dirname(dest_copy);
    char tmp_path[COPY_ARGS_MAX_VALUE];
    snprintf(tmp_path, sizeof(tmp_path), "%s/.ancible_copy_XXXXXX", dir);

    int fd = mkstemp(tmp_path);
    if (fd < 0) {
        return ANCIBLE_ERROR;
    }

    size_t written = 0;
    while (written < size) {
        ssize_t n = write(fd, data + written, size - written);
        if (n < 0) {
            close(fd);
            unlink(tmp_path);
            return ANCIBLE_ERROR;
        }
        written += (size_t)n;
    }

    if (fchmod(fd, mode) != 0) {
        close(fd);
        unlink(tmp_path);
        return ANCIBLE_ERROR;
    }

    if (close(fd) != 0) {
        unlink(tmp_path);
        return ANCIBLE_ERROR;
    }

    if (rename(tmp_path, dest) != 0) {
        unlink(tmp_path);
        return ANCIBLE_ERROR;
    }

    return ANCIBLE_SUCCESS;
}

int copy_module_exec(context_t *context, const char *args, module_result_t *result) {
    if (!context || !result) {
        return ANCIBLE_ERROR;
    }

    module_result_init(result);

    copy_args_t copy_args;
    if (copy_args_parse(args, &copy_args) != ANCIBLE_SUCCESS) {
        result->failed = 1;
        set_msg(result, "Failed to parse copy module arguments");
        return ANCIBLE_SUCCESS;
    }

    if (!copy_args.dest || copy_args.dest[0] == '\0') {
        result->failed = 1;
        set_msg(result, "No dest specified");
        copy_args_free(&copy_args);
        return ANCIBLE_SUCCESS;
    }

    int has_src = copy_args.src && copy_args.src[0] != '\0';
    int has_content = copy_args.content != NULL;

    if (has_src && has_content) {
        result->failed = 1;
        set_msg(result, "Specify either src or content, not both");
        copy_args_free(&copy_args);
        return ANCIBLE_SUCCESS;
    }

    if (!has_src && !has_content) {
        result->failed = 1;
        set_msg(result, "No src or content specified");
        copy_args_free(&copy_args);
        return ANCIBLE_SUCCESS;
    }

    mode_t mode = 0644;
    int has_mode = 0;

    if (copy_args.mode) {
        if (parse_mode(copy_args.mode, &mode) != ANCIBLE_SUCCESS) {
            result->failed = 1;
            set_msg(result, "Invalid mode: %s", copy_args.mode);
            copy_args_free(&copy_args);
            return ANCIBLE_SUCCESS;
        }
        has_mode = 1;
    }

    int force = parse_force(copy_args.force);

    char *payload = NULL;
    size_t payload_size = 0;

    if (has_content) {
        payload = strdup(copy_args.content);
        if (!payload) {
            result->failed = 1;
            set_msg(result, "Failed to allocate content buffer");
            copy_args_free(&copy_args);
            return ANCIBLE_SUCCESS;
        }
        payload_size = strlen(payload);
    } else {
        if (read_file(copy_args.src, &payload, &payload_size) != ANCIBLE_SUCCESS) {
            result->failed = 1;
            set_msg(result, "Failed to read source file %s", copy_args.src);
            copy_args_free(&copy_args);
            return ANCIBLE_SUCCESS;
        }
    }

    char *dest = resolve_dest(copy_args.dest, has_src ? copy_args.src : NULL);
    if (!dest) {
        result->failed = 1;
        set_msg(result, "Failed to resolve destination path");
        free(payload);
        copy_args_free(&copy_args);
        return ANCIBLE_SUCCESS;
    }

    int changed = 0;
    struct stat st;
    int dest_exists = (stat(dest, &st) == 0);

    if (dest_exists && S_ISDIR(st.st_mode)) {
        result->failed = 1;
        set_msg(result, "Destination %s is a directory", dest);
        free(payload);
        free(dest);
        copy_args_free(&copy_args);
        return ANCIBLE_SUCCESS;
    }

    if (dest_exists) {
        char *existing = NULL;
        size_t existing_size = 0;
        int same_content = 0;

        if (read_file(dest, &existing, &existing_size) == ANCIBLE_SUCCESS) {
            same_content = (existing_size == payload_size &&
                            memcmp(existing, payload, payload_size) == 0);
            free(existing);
        }

        if (same_content) {
            if (has_mode) {
                int mode_changed = 0;
                if (apply_mode(dest, mode, &mode_changed) != ANCIBLE_SUCCESS) {
                    result->failed = 1;
                    set_msg(result, "Failed to set mode on %s", dest);
                } else {
                    changed = mode_changed;
                    set_msg(result, changed ? "Updated mode on %s" : "%s is already up to date", dest);
                }
            } else {
                set_msg(result, "%s is already up to date", dest);
            }
        } else if (!force) {
            result->failed = 1;
            set_msg(result, "Destination %s already exists and force=no", dest);
        } else {
            if (write_file(dest, payload, payload_size, has_mode ? mode : (st.st_mode & 07777)) != ANCIBLE_SUCCESS) {
                result->failed = 1;
                set_msg(result, "Failed to write %s", dest);
            } else {
                changed = 1;
                set_msg(result, "Copied to %s", dest);
            }
        }
    } else {
        if (write_file(dest, payload, payload_size, mode) != ANCIBLE_SUCCESS) {
            result->failed = 1;
            set_msg(result, "Failed to write %s", dest);
        } else {
            changed = 1;
            set_msg(result, "Copied to %s", dest);
        }
    }

    result->changed = changed;
    free(payload);
    free(dest);
    copy_args_free(&copy_args);
    return ANCIBLE_SUCCESS;
}
