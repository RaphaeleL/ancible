#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../include/ancible.h"
#include "../include/modules/template.h"

#define TEMPLATE_VALUE_MAX 4096

typedef struct {
    char *src;
    char *dest;
    char *mode;
} template_args_t;

static void set_msg(module_result_t *result, const char *fmt, ...) {
    va_list ap;
    free(result->msg);
    result->msg = malloc(512);
    if (!result->msg) return;
    va_start(ap, fmt);
    vsnprintf(result->msg, 512, fmt, ap);
    va_end(ap);
}

static const char *next_kv(const char *p, char *key, size_t key_size,
                           char *value, size_t value_size) {
    while (p && *p && isspace((unsigned char)*p)) p++;
    if (!p || !*p) return NULL;

    size_t n = 0;
    while (*p && *p != '=' && !isspace((unsigned char)*p) && n + 1 < key_size)
        key[n++] = *p++;
    key[n] = '\0';
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '=') return p;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;

    char quote = 0;
    if (*p == '\'' || *p == '"') quote = *p++;
    n = 0;
    while (*p && n + 1 < value_size) {
        if (quote && *p == quote) { p++; break; }
        if (!quote && isspace((unsigned char)*p)) break;
        if (*p == '\\' && quote && p[1]) p++;
        value[n++] = *p++;
    }
    value[n] = '\0';
    while (*p && isspace((unsigned char)*p)) p++;
    return *p ? p : NULL;
}

static void free_args(template_args_t *args) {
    free(args->src);
    free(args->dest);
    free(args->mode);
    memset(args, 0, sizeof(*args));
}

static int parse_args(const char *input, template_args_t *args) {
    char key[64], value[TEMPLATE_VALUE_MAX];
    memset(args, 0, sizeof(*args));
    for (const char *p = input; p;) {
        p = next_kv(p, key, sizeof(key), value, sizeof(value));
        if (strcmp(key, "src") == 0) args->src = strdup(value);
        else if (strcmp(key, "dest") == 0) args->dest = strdup(value);
        else if (strcmp(key, "mode") == 0) args->mode = strdup(value);
    }
    return ANCIBLE_SUCCESS;
}

static int read_file(const char *path, char **data, size_t *size) {
    FILE *fp = fopen(path, "rb");
    if (!fp || fseek(fp, 0, SEEK_END) != 0) { if (fp) fclose(fp); return ANCIBLE_ERROR; }
    long length = ftell(fp);
    if (length < 0 || fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return ANCIBLE_ERROR; }
    *data = malloc((size_t)length + 1);
    if (!*data) { fclose(fp); return ANCIBLE_ERROR; }
    *size = fread(*data, 1, (size_t)length, fp);
    (*data)[*size] = '\0';
    if (ferror(fp)) { free(*data); *data = NULL; fclose(fp); return ANCIBLE_ERROR; }
    fclose(fp);
    return ANCIBLE_SUCCESS;
}

static int parse_mode(const char *text, mode_t *mode) {
    char *end = NULL;
    long value;
    if (!text) return ANCIBLE_ERROR;
    value = strtol(text, &end, 8);
    if (end == text || *end || value < 0 || value > 07777) return ANCIBLE_ERROR;
    *mode = (mode_t)value;
    return ANCIBLE_SUCCESS;
}

static int write_file(const char *path, const char *data, size_t size, mode_t mode) {
    char temp[TEMPLATE_VALUE_MAX];
    int fd;
    if (snprintf(temp, sizeof(temp), "%s.ancible_template.XXXXXX", path) >= (int)sizeof(temp))
        return ANCIBLE_ERROR;
    fd = mkstemp(temp);
    if (fd < 0) return ANCIBLE_ERROR;
    if (fchmod(fd, mode) != 0) { close(fd); unlink(temp); return ANCIBLE_ERROR; }
    size_t written = 0;
    while (written < size) {
        ssize_t n = write(fd, data + written, size - written);
        if (n <= 0) { close(fd); unlink(temp); return ANCIBLE_ERROR; }
        written += (size_t)n;
    }
    if (close(fd) != 0 || rename(temp, path) != 0) { unlink(temp); return ANCIBLE_ERROR; }
    return ANCIBLE_SUCCESS;
}

int template_module_exec(context_t *context, const char *args, module_result_t *result) {
    template_args_t parsed;
    char *source = NULL, *rendered = NULL;
    size_t source_size = 0, rendered_size;
    mode_t mode = 0644;
    struct stat st;

    if (!context || !result) return ANCIBLE_ERROR;
    module_result_init(result);
    parse_args(args ? args : "", &parsed);
    if (!parsed.src || !parsed.dest) {
        result->failed = 1;
        set_msg(result, "Template requires src and dest");
        free_args(&parsed);
        return ANCIBLE_SUCCESS;
    }
    if (parsed.mode && parse_mode(parsed.mode, &mode) != ANCIBLE_SUCCESS) {
        result->failed = 1;
        set_msg(result, "Invalid mode: %s", parsed.mode);
        free_args(&parsed);
        return ANCIBLE_SUCCESS;
    }
    if (read_file(parsed.src, &source, &source_size) != ANCIBLE_SUCCESS) {
        result->failed = 1;
        set_msg(result, "Failed to read template %s", parsed.src);
        free_args(&parsed);
        return ANCIBLE_SUCCESS;
    }
    rendered = context_render_template(context, source);
    rendered_size = rendered ? strlen(rendered) : 0;
    free(source);
    if (!rendered) {
        result->failed = 1;
        set_msg(result, "Failed to render template %s", parsed.src);
        free_args(&parsed);
        return ANCIBLE_SUCCESS;
    }

    if (stat(parsed.dest, &st) == 0 && S_ISDIR(st.st_mode)) {
        result->failed = 1;
        set_msg(result, "Destination %s is a directory", parsed.dest);
    } else if (stat(parsed.dest, &st) == 0) {
        char *old = NULL; size_t old_size = 0;
        int same = read_file(parsed.dest, &old, &old_size) == ANCIBLE_SUCCESS &&
                   old_size == rendered_size && memcmp(old, rendered, rendered_size) == 0;
        free(old);
        if (same && (!parsed.mode || (st.st_mode & 07777) == mode)) {
            set_msg(result, "%s is already up to date", parsed.dest);
        } else if (write_file(parsed.dest, rendered, rendered_size,
                              parsed.mode ? mode : (st.st_mode & 07777)) != ANCIBLE_SUCCESS) {
            result->failed = 1; set_msg(result, "Failed to write %s", parsed.dest);
        } else { result->changed = 1; set_msg(result, "Rendered to %s", parsed.dest); }
    } else if (write_file(parsed.dest, rendered, rendered_size, mode) != ANCIBLE_SUCCESS) {
        result->failed = 1; set_msg(result, "Failed to write %s", parsed.dest);
    } else { result->changed = 1; set_msg(result, "Rendered to %s", parsed.dest); }

    free(rendered);
    free_args(&parsed);
    return ANCIBLE_SUCCESS;
}
