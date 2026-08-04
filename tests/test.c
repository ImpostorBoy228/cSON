#define _XOPEN_SOURCE 700
#define son😭😭😭😭
#include "../src/cSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks = 0;
static int fails = 0;

#define CHECK(cond) do {                                          \
    checks++;                                                     \
    if (!(cond)) {                                                \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        fails++;                                                  \
    }                                                             \
} while (0)

static char dir[] = "cson_test_XXXXXX";

static const char* write_fixture(const char* name, const char* content) {
    static char path[1024];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    FILE* f = fopen(path, "w");
    if (!f) { perror("fopen"); exit(1); }
    fputs(content, f);
    fclose(f);
    return path;
}

static cSON_Obj* parse_ok(const char* name, const char* content) {
    cSON_Obj* root = NULL;
    CHECK(cSON_parse(&root, write_fixture(name, content)) == 1);
    CHECK(root != NULL);
    return root;
}

static void test_basic(void) {
    cSON_Obj* root = parse_ok("basic.son",
        "{\n"
        "    \"a\": \"bubububu\"\n"
        "    \"b\": \"hello\"\n"
        "    \"c\": {\n"
        "        \"d\": \"e\"\n"
        "    }\n"
        "}\n");
    if (!root) return;
    const char* v = cSON_get(root, "a");
    CHECK(v && strcmp(v, "bubububu") == 0);
    CHECK(cSON_get(root, "b") && strcmp(cSON_get(root, "b"), "hello") == 0);
    const cSON_Obj* c = cSON_find(root, "c");
    CHECK(c && c->type == CSON_OBJECT);
    CHECK(cSON_get(c, "d") && strcmp(cSON_get(c, "d"), "e") == 0);
    CHECK(cSON_get(root, "nope") == NULL);
    cSON_free(root);
}

static void test_if(void) {
    cSON_Obj* root = parse_ok("if.son",
        "{\n"
        "    \"a\": {\n"
        "        !\"if(owo == \"dick\")\": {\n"
        "            \"text\": \"Hello\"\n"
        "        }\n"
        "    }\n"
        "}\n");
    if (!root) return;
    const cSON_Obj* a = cSON_find(root, "a");
    CHECK(a && a->type == CSON_OBJECT);
    const cSON_Obj* ifn = a ? a->child : NULL;
    CHECK(ifn && ifn->type == CSON_IF);
    CHECK(ifn->value && strcmp(ifn->value, "if(owo == \"dick\")") == 0);
    CHECK(cSON_get(ifn, "text") == NULL);
    CHECK(ifn->child && cSON_get(ifn->child, "text") && strcmp(cSON_get(ifn->child, "text"), "Hello") == 0);
    cSON_free(root);
}

static void test_escapes(void) {
    cSON_Obj* root = parse_ok("escapes.son",
        "{\n"
        "    \"q\": \"say \\\"hi\\\"\"\n"
        "    \"s\": \"a\\\\b\"\n"
        "}\n");
    if (!root) return;
    CHECK(cSON_get(root, "q") && strcmp(cSON_get(root, "q"), "say \"hi\"") == 0);
    CHECK(cSON_get(root, "s") && strcmp(cSON_get(root, "s"), "a\\b") == 0);
    cSON_free(root);
}

static void test_comments(void) {
    cSON_Obj* root = parse_ok("comments.son",
        "{\n"
        "; comment\n"
        "    \"k\": \"v\" ; trailing\n"
        "}\n");
    if (!root) return;
    CHECK(cSON_get(root, "k") && strcmp(cSON_get(root, "k"), "v") == 0);
    cSON_free(root);
}

static void test_orphan(void) {
    cSON_Obj* root = parse_ok("orphan.son",
        "{\n"
        "    \"k\": \"v\"\n"
        "    \"orphan\"\n"
        "}\n");
    if (!root) return;
    CHECK(cSON_get(root, "k") && strcmp(cSON_get(root, "k"), "v") == 0);
    cSON_free(root);
}

static void test_macro(void) {
    cSON_Obj* root = parse_ok("macro.son",
        "#define VAR dick\n"
        "{\n"
        "    \"cock\": \"VAR\"\n"
        "}\n");
    if (!root) return;
    CHECK(cSON_get(root, "cock") && strcmp(cSON_get(root, "cock"), "dick") == 0);
    cSON_free(root);
}

static void test_macro_if(void) {
    cSON_Obj* root = parse_ok("macro_if.son",
        "#define VAR dick\n"
        "{\n"
        "    \"a\": {\n"
        "        !\"if(owo == \"VAR\")\": {\n"
        "            \"text\": \"Hello owo!!\"\n"
        "        }\n"
        "    }\n"
        "}\n");
    if (!root) return;
    const cSON_Obj* a = cSON_find(root, "a");
    const cSON_Obj* ifn = a ? a->child : NULL;
    CHECK(ifn && ifn->type == CSON_IF);
    CHECK(ifn->value && strcmp(ifn->value, "if(owo == \"dick\")") == 0);
    cSON_free(root);
}

static char* read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    buf[rd] = '\0';
    fclose(f);
    return buf;
}

static void test_apply_if_unfold(void) {
    cSON_Obj* root = parse_ok("apply1.son",
        "{\n"
        "    \"a\": {\n"
        "        !\"if(x == 1)\": {\n"
        "            \"b\": \"v\"\n"
        "        }\n"
        "    }\n"
        "}\n");
    if (!root) return;
    cSON_apply_if(root, 0, 1);
    const cSON_Obj* a = cSON_find(root, "a");
    CHECK(a != NULL);
    int has_if = 0;
    for (const cSON_Obj* c = a ? a->child : NULL; c; c = c->next)
        if (c->type == CSON_IF) has_if = 1;
    CHECK(!has_if);
    CHECK(cSON_get(a, "b") && strcmp(cSON_get(a, "b"), "v") == 0);
    cSON_free(root);
}

static void test_apply_if_delete(void) {
    cSON_Obj* root = parse_ok("apply2.son",
        "{\n"
        "    \"a\": {\n"
        "        !\"if(x == 1)\": {\n"
        "            \"b\": \"v\"\n"
        "        }\n"
        "    }\n"
        "}\n");
    if (!root) return;
    cSON_apply_if(root, 0, 0);
    const cSON_Obj* a = cSON_find(root, "a");
    CHECK(a != NULL);
    CHECK(a->child == NULL);
    CHECK(cSON_get(a, "b") == NULL);
    cSON_free(root);
}

static void test_apply_if_out_of_range(void) {
    cSON_Obj* root = parse_ok("apply3.son",
        "{\n"
        "    \"a\": {\n"
        "        !\"if(x == 1)\": {\n"
        "            \"b\": \"v\"\n"
        "        }\n"
        "    }\n"
        "}\n");
    if (!root) return;
    cSON_apply_if(root, 99, 1);
    const cSON_Obj* a = cSON_find(root, "a");
    CHECK(a != NULL);
    const cSON_Obj* ifn = a ? a->child : NULL;
    CHECK(ifn && ifn->type == CSON_IF);
    cSON_free(root);
}

static void test_gen_evalpoint(void) {
    const char* son = write_fixture("gen.son",
        "#define VAR dick\n"
        "{\n"
        "    \"a\": {\n"
        "        !\"if(owo == \"VAR\")\": {\n"
        "            \"text\": \"Hello owo!!\"\n"
        "        }\n"
        "    }\n"
        "}\n");
    char out[1024];
    snprintf(out, sizeof out, "%s/SONEVAL.ass", dir);
    CHECK(cSON_gen_evalpoint(out, son) == 1);

    char* buf = read_file(out);
    CHECK(buf != NULL);
    if (!buf) return;
    CHECK(strstr(buf, "#define cSON_evalpoint()") != NULL);
    CHECK(strstr(buf, "if(owo == \"dick\")") != NULL);
    CHECK(strstr(buf, "cSON_apply_if(_son_root, 0, 1)") != NULL);
    CHECK(strstr(buf, son) != NULL);
    free(buf);
}

static void test_empty(void) {
    cSON_Obj* root = NULL;
    CHECK(cSON_parse(&root, write_fixture("empty.son", "")) == 1);
    CHECK(root == NULL);
    if (root) cSON_free(root);
}

static void test_missing(void) {
    cSON_Obj* root = NULL;
    CHECK(cSON_parse(&root, "/no/such/file/x.son") == 0);
    CHECK(root == NULL);
}

int main(void) {
    if (!mkdtemp(dir)) { perror("mkdtemp"); return 1; }

    test_basic();
    test_if();
    test_escapes();
    test_comments();
    test_orphan();
    test_macro();
    test_macro_if();
    test_apply_if_unfold();
    test_apply_if_delete();
    test_apply_if_out_of_range();
    test_gen_evalpoint();
    test_empty();
    test_missing();

    char cmd[1024];
    snprintf(cmd, sizeof cmd, "rm -rf %s", dir);
    system(cmd);

    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
