#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../../include/ancible.h"
#include "../../include/core/context.h"
#include "../../include/modules/module.h"
#include "../../include/modules/file.h"

#define TEST_DIR_PATH "/tmp/ancible_file_test"

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
    playbook->tasks[0].name = strdup("Test file task");
    playbook->tasks[0].module = strdup("file");

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
 * Clean up test directory
 */
static void cleanup_test_dir(void) {
    char path[256];
    snprintf(path, sizeof(path), "%s/file.txt", TEST_DIR_PATH);
    unlink(path);
    snprintf(path, sizeof(path), "%s/nested", TEST_DIR_PATH);
    rmdir(path);
    rmdir(TEST_DIR_PATH);
}

/**
 * Test for file module functionality
 */
int main(void) {
    printf("Running file module tests\n");

    cleanup_test_dir();

    // Test 1: Create a directory
    {
        printf("Test 1: Creating a directory... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[256];
        snprintf(args, sizeof(args), "path=%s state=directory mode=0755", TEST_DIR_PATH);

        module_result_t result;
        int ret = file_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 1);

        struct stat st;
        assert(stat(TEST_DIR_PATH, &st) == 0);
        assert(S_ISDIR(st.st_mode));
        assert((st.st_mode & 07777) == 0755);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 2: Create a file with mode
    {
        printf("Test 2: Creating a file with mode... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[256];
        snprintf(args, sizeof(args), "path=%s/file.txt state=present mode=0644", TEST_DIR_PATH);

        module_result_t result;
        int ret = file_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 1);

        struct stat st;
        assert(stat(TEST_DIR_PATH "/file.txt", &st) == 0);
        assert(S_ISREG(st.st_mode));
        assert((st.st_mode & 07777) == 0644);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 3: Idempotent file creation
    {
        printf("Test 3: Idempotent file creation... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[256];
        snprintf(args, sizeof(args), "path=%s/file.txt state=present", TEST_DIR_PATH);

        module_result_t result;
        int ret = file_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 0);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 4: Change mode (chmod)
    {
        printf("Test 4: Changing file mode... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[256];
        snprintf(args, sizeof(args), "path=%s/file.txt state=present mode=0600", TEST_DIR_PATH);

        module_result_t result;
        int ret = file_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 1);

        struct stat st;
        assert(stat(TEST_DIR_PATH "/file.txt", &st) == 0);
        assert((st.st_mode & 07777) == 0600);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 5: Delete a file
    {
        printf("Test 5: Deleting a file... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[256];
        snprintf(args, sizeof(args), "path=%s/file.txt state=absent", TEST_DIR_PATH);

        module_result_t result;
        int ret = file_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 1);
        assert(access(TEST_DIR_PATH "/file.txt", F_OK) != 0);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 6: Idempotent deletion
    {
        printf("Test 6: Idempotent file deletion... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        char args[256];
        snprintf(args, sizeof(args), "path=%s/file.txt state=absent", TEST_DIR_PATH);

        module_result_t result;
        int ret = file_module_exec(context, args, &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 0);
        assert(result.changed == 0);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    // Test 7: Missing path fails
    {
        printf("Test 7: Missing path fails... ");

        host_t *host = create_test_host();
        playbook_t *playbook = create_test_playbook();
        context_t *context = context_create(host, playbook, 0);
        assert(context != NULL);
        context_set_var(context, "ansible_connection", "local");

        module_result_t result;
        int ret = file_module_exec(context, "state=present", &result);

        assert(ret == ANCIBLE_SUCCESS);
        assert(result.failed == 1);

        module_result_free(&result);
        context_free(context);
        free_test_host(host);
        free_test_playbook(playbook);

        printf("OK\n");
    }

    cleanup_test_dir();

    printf("All file module tests passed!\n");
    return 0;
}
