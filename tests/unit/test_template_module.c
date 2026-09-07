#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../../include/ancible.h"
#include "../../include/core/context.h"
#include "../../include/modules/module.h"
#include "../../include/modules/template.h"

#define TEST_DIR "/tmp/ancible_template_test"

static host_t *host_create(void) {
    host_t *host = calloc(1, sizeof(*host));
    assert(host);
    host->name = strdup("localhost");
    host->ansible_host = strdup("localhost");
    return host;
}

static void host_free(host_t *host) {
    free(host->name); free(host->ansible_host); free(host);
}

static playbook_t *playbook_create(void) {
    playbook_t *playbook = calloc(1, sizeof(*playbook));
    assert(playbook);
    playbook->hosts = strdup("all");
    playbook->tasks = calloc(1, sizeof(task_t));
    playbook->task_count = 1;
    playbook->tasks[0].parent_idx = -1;
    return playbook;
}

static void playbook_free_test(playbook_t *playbook) {
    free(playbook->hosts); free(playbook->tasks); free(playbook);
}

static void write_text(const char *path, const char *text) {
    FILE *fp = fopen(path, "w");
    assert(fp); fputs(text, fp); fclose(fp);
}

static char *read_text(const char *path) {
    FILE *fp = fopen(path, "r");
    char buf[512]; size_t n;
    assert(fp); n = fread(buf, 1, sizeof(buf) - 1, fp); fclose(fp);
    buf[n] = '\0'; return strdup(buf);
}

int main(void) {
    host_t *host;
    playbook_t *playbook;
    context_t *context;
    module_result_t result;
    char *output;

    printf("Running template module tests\n");
    mkdir(TEST_DIR, 0755);
    unlink(TEST_DIR "/rendered.txt");
    write_text(TEST_DIR "/template.txt", "Hello {{ name | upper }}!\n");

    host = host_create(); playbook = playbook_create();
    context = context_create(host, playbook, 0); assert(context);
    context_set_var(context, "name", "ancible");

    printf("Test 1: Rendering template... ");
    assert(template_module_exec(context, "src=" TEST_DIR "/template.txt dest=" TEST_DIR "/rendered.txt mode=0600", &result) == ANCIBLE_SUCCESS);
    assert(result.failed == 0 && result.changed == 1);
    output = read_text(TEST_DIR "/rendered.txt");
    assert(strcmp(output, "Hello ANCIBLE!\n") == 0); free(output);
    module_result_free(&result); printf("OK\n");

    printf("Test 2: Idempotent rendering... ");
    assert(template_module_exec(context, "src=" TEST_DIR "/template.txt dest=" TEST_DIR "/rendered.txt mode=0600", &result) == ANCIBLE_SUCCESS);
    assert(result.failed == 0 && result.changed == 0);
    module_result_free(&result); printf("OK\n");

    printf("Test 3: Missing source fails... ");
    assert(template_module_exec(context, "src=/tmp/no-such-template dest=" TEST_DIR "/rendered.txt", &result) == ANCIBLE_SUCCESS);
    assert(result.failed == 1 && result.changed == 0);
    module_result_free(&result); printf("OK\n");

    context_free(context); host_free(host); playbook_free_test(playbook);
    unlink(TEST_DIR "/template.txt"); unlink(TEST_DIR "/rendered.txt"); rmdir(TEST_DIR);
    printf("All template module tests passed!\n");
    return 0;
}
