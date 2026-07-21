#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../../include/ancible.h"
#include "../../include/core/context.h"
#include "../../include/modules/module.h"
#include "../../include/modules/copy.h"

#define TEST_DIR_PATH "/tmp/ancible_copy_test"

/**
 * Create a test host
 */
static host_t *create_test_host(void) {
    host_t *host = malloc(sizeof(host_t));
    assert(host != NULL);

    host->name = strdup("localhost");
    host->ansible_host = strdup("localhost");
    host->next = NULL;

    return host;
}

/**
 * Free a test host
 */
static void free_test_host(host_t *host) {
    if (!host) return;

    free(host->name);
    free(host->ansible_host);
    free(host);
}

/**
 * Create a test playbook
 */
static playbook_t *create_test_playbook(void) {
    playbook_t *playbook = malloc(sizeof(playbook_t));
    assert(playbook != NULL);

    playbook->hosts = strdup("all");
    playbook->task_count = 1;

    playbook->tasks = malloc(sizeof(task_t));
    memset(&playbook->tasks[0], 0, sizeof(task_t));
    playbook->tasks[0].parent_idx = -1;
    playbook->tasks[0].name = strdup("Test copy task");
    playbook->tasks[0].module = strdup("copy");

    return playbook;
}

/**
 * Free a test playbook
 */
static void free_test_playbook(playbook_t *playbook) {
    if (!playbook) return;

    free(playbook->hosts);

    for (int i = 0; i < playbook->task_count; i++) {
        free(playbook->tasks[i].name);
        free(playbook->tasks[i].module);
        free(playbook->tasks[i].when);
        free(playbook->tasks[i].register_var);
        free(playbook->tasks[i].subtask_indices);
    }

    free(playbook->tasks);
    free(playbook);
}

/**
 * Clean up test directory and files
 */
static void cleanup_test_dir(void) {
    char path[256];
    snprintf(path, sizeof(path), "%s/source.txt", TEST_DIR_PATH);
    unlink(path);
    snprintf(path, sizeof(path), "%s/dest.txt", TEST_DIR_PATH);
    unlink(path);
    snprintf(path, sizeof(path), "%s/from_content.txt", TEST_DIR_PATH);
    unlink(path);
    snprintf(path, sizeof(path), "%s/into_dir/source.txt", TEST_DIR_PATH);
    unlink(path);
    snprintf(path, sizeof(path), "%s/into_dir", TEST_DIR_PATH);
    rmdir(path);
    rmdir(TEST_DIR_PATH);
}

/**
 * Write a helper source file
 */
static void write_source_file(const char *path, const char *content) {
    FILE *fp = fopen(path, "w");
    assert(fp != NULL);
    fputs(content, fp);
    fclose(fp);
}

/**
 * Read a file into a fixed buffer for assertions
 */
static void read_file_assert(const char *path, char *buf, size_t buf_size) {
    FILE *fp = fopen(path, "r");
    assert(fp != NULL);
    size_t n = fread(buf, 1, buf_size - 1, fp);
    buf[n] = '\0';
    fclose(fp);
}

int main(void) {
    printf("Running copy module tests\n");

    cleanup_test_dir();
    assert(mkdir(TEST_DIR_PATH, 0755) == 0);

    // Test 1: Copy from content=
    {
        printf("Test 1: Copy from content... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[512];
        snprintf(args, sizeof(args),
                 "content='Hello from copy' dest=%s/from_content.txt mode=0644",
                 TEST_DIR_PATH);

        module_result_t result;
        int ret = copy_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 1);

        char buf[256];
        read_file_assert(TEST_DIR_PATH "/from_content.txt", buf, sizeof(buf));
        assert(strcmp(buf, "Hello from copy") == 0);

        struct stat st;
        assert(stat(TEST_DIR_PATH "/from_content.txt", &st) == 0);
        assert((st.st_mode & 07777) == 0644);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 2: Idempotent content copy
    {
        printf("Test 2: Idempotent content copy... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[512];
        snprintf(args, sizeof(args),
                 "content='Hello from copy' dest=%s/from_content.txt",
                 TEST_DIR_PATH);

        module_result_t result;
        int ret = copy_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 0);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 3: Copy from src=
    {
        printf("Test 3: Copy from src... ");

        write_source_file(TEST_DIR_PATH "/source.txt", "Source file payload\n");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[512];
        snprintf(args, sizeof(args),
                 "src=%s/source.txt dest=%s/dest.txt mode=0600",
                 TEST_DIR_PATH, TEST_DIR_PATH);

        module_result_t result;
        int ret = copy_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 1);

        char buf[256];
        read_file_assert(TEST_DIR_PATH "/dest.txt", buf, sizeof(buf));
        assert(strcmp(buf, "Source file payload\n") == 0);

        struct stat st;
        assert(stat(TEST_DIR_PATH "/dest.txt", &st) == 0);
        assert((st.st_mode & 07777) == 0600);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 4: Update existing file content
    {
        printf("Test 4: Update existing file content... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[512];
        snprintf(args, sizeof(args),
                 "content='Updated payload' dest=%s/dest.txt",
                 TEST_DIR_PATH);

        module_result_t result;
        int ret = copy_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 1);

        char buf[256];
        read_file_assert(TEST_DIR_PATH "/dest.txt", buf, sizeof(buf));
        assert(strcmp(buf, "Updated payload") == 0);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 5: force=no refuses overwrite
    {
        printf("Test 5: force=no refuses overwrite... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[512];
        snprintf(args, sizeof(args),
                 "content='Should not write' dest=%s/dest.txt force=no",
                 TEST_DIR_PATH);

        module_result_t result;
        int ret = copy_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 1);
        assert(result.changed == 0);

        char buf[256];
        read_file_assert(TEST_DIR_PATH "/dest.txt", buf, sizeof(buf));
        assert(strcmp(buf, "Updated payload") == 0);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 6: Copy into existing directory uses basename
    {
        printf("Test 6: Copy into directory... ");

        assert(mkdir(TEST_DIR_PATH "/into_dir", 0755) == 0);

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[512];
        snprintf(args, sizeof(args),
                 "src=%s/source.txt dest=%s/into_dir",
                 TEST_DIR_PATH, TEST_DIR_PATH);

        module_result_t result;
        int ret = copy_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 1);
        assert(access(TEST_DIR_PATH "/into_dir/source.txt", F_OK) == 0);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 7: Missing dest fails
    {
        printf("Test 7: Missing dest fails... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        module_result_t result;
        int ret = copy_module_exec(context, "content='no dest'", &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 1);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 8: Missing src and content fails
    {
        printf("Test 8: Missing src and content fails... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[256];
        snprintf(args, sizeof(args), "dest=%s/missing.txt", TEST_DIR_PATH);

        module_result_t result;
        int ret = copy_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 1);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    cleanup_test_dir();

    printf("All copy module tests passed!\n");
    return 0;
}
