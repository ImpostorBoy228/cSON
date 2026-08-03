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
    test_empty();
    test_missing();

    char cmd[1024];
    snprintf(cmd, sizeof cmd, "rm -rf %s", dir);
    system(cmd);

    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
