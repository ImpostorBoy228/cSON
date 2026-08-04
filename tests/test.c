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

static void test_inline_c(void) {
    cSON_Obj* root = parse_ok("inline_c.son",
        "{\n"
        "    \"res\": {\n"
        "        !!\"__c__\": \"char* poo(){return \\\"998\\\";}\"\n"
        "    }\n"
        "}\n");
    if (!root) return;
    const cSON_Obj* res = cSON_find(root, "res");
    CHECK(res && res->type == CSON_OBJECT);
    const cSON_Obj* in = res ? res->child : NULL;
    CHECK(in && in->type == CSON_INLINE_C);
    CHECK(in->key == NULL);
    CHECK(in->value && strcmp(in->value, "char* poo(){return \"998\";}") == 0);
    cSON_free(root);
}

static void test_inline_asm(void) {
    cSON_Obj* root = parse_ok("inline_asm.son",
        "{\n"
        "    \"x\": {\n"
        "        !!\"__asm__\": \"char* ares(){return \\\"42\\\";}\"\n"
        "    }\n"
        "}\n");
    if (!root) return;
    const cSON_Obj* x = cSON_find(root, "x");
    const cSON_Obj* in = x ? x->child : NULL;
    CHECK(in && in->type == CSON_INLINE_ASM);
    CHECK(in->key == NULL);
    CHECK(in->value && strcmp(in->value, "char* ares(){return \"42\";}") == 0);
    cSON_free(root);
}

static void test_brace_same_line(void) {
    cSON_Obj* root = parse_ok("brace_same_line.son",
        "{\"a\":{\"b\":\"v\"}}");
    if (!root) return;
    const cSON_Obj* a = cSON_find(root, "a");
    CHECK(a && a->type == CSON_OBJECT);
    CHECK(cSON_get(a, "b") && strcmp(cSON_get(a, "b"), "v") == 0);
    cSON_free(root);
}

static void test_brace_empty(void) {
    cSON_Obj* root = parse_ok("brace_empty.son",
        "{\"a\":{}}");
    if (!root) return;
    const cSON_Obj* a = cSON_find(root, "a");
    CHECK(a && a->type == CSON_OBJECT);
    CHECK(a->child == NULL);
    cSON_free(root);
}

static void test_brace_closing_newline(void) {
    cSON_Obj* root = parse_ok("brace_closing_nl.son",
        "{ \"a\": { \"b\": \"v\" }\n}");
    if (!root) return;
    const cSON_Obj* a = cSON_find(root, "a");
    CHECK(a && a->type == CSON_OBJECT);
    CHECK(cSON_get(a, "b") && strcmp(cSON_get(a, "b"), "v") == 0);
    cSON_free(root);
}

static void test_brace_blank_lines(void) {
    cSON_Obj* root = parse_ok("brace_blank.son",
        "{ \"a\": { \"b\": \"v\" }\n\n\n}\n\n\n");
    if (!root) return;
    const cSON_Obj* a = cSON_find(root, "a");
    CHECK(a && a->type == CSON_OBJECT);
    CHECK(cSON_get(a, "b") && strcmp(cSON_get(a, "b"), "v") == 0);
    cSON_free(root);
}

static void test_brace_adjacent_siblings(void) {
    cSON_Obj* root = parse_ok("brace_sib.son",
        "{\"a\":{\"b\":{\"c\":\"v\"}} \"x\":\"y\"}");
    if (!root) return;
    const cSON_Obj* a = cSON_find(root, "a");
    const cSON_Obj* b = a ? cSON_find(a, "b") : NULL;
    CHECK(b && b->type == CSON_OBJECT);
    CHECK(cSON_get(b, "c") && strcmp(cSON_get(b, "c"), "v") == 0);
    CHECK(cSON_get(root, "x") && strcmp(cSON_get(root, "x"), "y") == 0);
    cSON_free(root);
}

static void test_crlf(void) {
    cSON_Obj* root = parse_ok("crlf.son",
        "{\r\n"
        "    \"a\": \"bubububu\"\r\n"
        "    \"c\": {\r\n"
        "        \"d\": \"e\"\r\n"
        "    }\r\n"
        "}\r\n");
    if (!root) return;
    const char* v = cSON_get(root, "a");
    CHECK(v && strcmp(v, "bubububu") == 0);
    const cSON_Obj* c = cSON_find(root, "c");
    CHECK(c && c->type == CSON_OBJECT);
    CHECK(cSON_get(c, "d") && strcmp(cSON_get(c, "d"), "e") == 0);
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

static void test_if_siblings(void) {
    cSON_Obj* root = parse_ok("if_siblings.son",
        "{\n"
        "    \"a\": {\n"
        "        !\"if(x == 1)\": {\n"
        "            \"b\": \"v\"\n"
        "        }\n"
        "        !\"if(y == 2)\": {\n"
        "            \"c\": \"w\"\n"
        "        }\n"
        "    }\n"
        "}\n");
    if (!root) return;
    const cSON_Obj* a = cSON_find(root, "a");
    CHECK(a != NULL);
    int ifc = 0;
    for (const cSON_Obj* c = a ? a->child : NULL; c; c = c->next)
        if (c->type == CSON_IF) ifc++;
    CHECK(ifc == 2);
    cSON_free(root);
}

static void test_if_siblings_apply(void) {
    cSON_Obj* root = parse_ok("if_sib_apply.son",
        "{\n"
        "    \"a\": {\n"
        "        !\"if(x == 1)\": {\n"
        "            \"b\": \"v\"\n"
        "        }\n"
        "        !\"if(y == 2)\": {\n"
        "            \"c\": \"w\"\n"
        "        }\n"
        "    }\n"
        "}\n");
    if (!root) return;
    cSON_apply_if(root, 1, 1);
    cSON_apply_if(root, 0, 1);
    const cSON_Obj* a = cSON_find(root, "a");
    CHECK(cSON_get(a, "b") && strcmp(cSON_get(a, "b"), "v") == 0);
    CHECK(cSON_get(a, "c") && strcmp(cSON_get(a, "c"), "w") == 0);
    cSON_free(root);
}

static void test_apply_inline_collapse(void) {
    cSON_Obj* root = parse_ok("inline_collapse.son",
        "{\n"
        "    \"res\": {\n"
        "        !!\"__c__\": \"char* poo(){return \\\"998\\\";}\"\n"
        "    }\n"
        "}\n");
    if (!root) return;
    cSON_apply_inline(root, 0, "998");
    root = cSON_collapse_inlines(root);
    CHECK(cSON_get(root, "res") && strcmp(cSON_get(root, "res"), "998") == 0);
    const cSON_Obj* res = cSON_find(root, "res");
    CHECK(res && res->type == CSON_STRING);
    cSON_free(root);
}

static void test_apply_inline_no_collapse(void) {
    cSON_Obj* root = parse_ok("inline_no_collapse.son",
        "{\n"
        "    \"res\": {\n"
        "        !!\"__c__\": \"char* poo(){return \\\"998\\\";}\"\n"
        "        \"x\": \"y\"\n"
        "    }\n"
        "}\n");
    if (!root) return;
    cSON_apply_inline(root, 0, "998");
    root = cSON_collapse_inlines(root);
    const cSON_Obj* res = cSON_find(root, "res");
    CHECK(res && res->type == CSON_OBJECT);
    CHECK(cSON_get(root, "res") == NULL);
    const cSON_Obj* in = res ? res->child : NULL;
    CHECK(in && in->type == CSON_INLINE_C);
    CHECK(in->value && strcmp(in->value, "998") == 0);
    cSON_free(root);
}

static void test_gen_evalpoint(void) {
    const char* son = write_fixture("gen.son",
        "#define VAR dick\n"
        "{\n"
        "    \"important code result\": {\n"
        "        !!\"__c__\": \"char* poo(){static char _b[32]; for(int d=0; d<999; d++){if (d==998){snprintf(_b,sizeof _b,\\\"%d\\\",d); return _b;}} return \\\"0\\\";}\"\n"
        "    }\n"
        "    \"assembler result\": {\n"
        "        !!\"__asm__\": \"char* ares(){int v; __asm__ volatile (\\\"mov $42, %0\\\" : \\\"=r\\\"(v)); static char _b[16]; snprintf(_b,sizeof _b,\\\"%d\\\",v); return _b;}\"\n"
        "    }\n"
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
    CHECK(strstr(buf, "char* poo()") != NULL);
    CHECK(strstr(buf, "char* ares()") != NULL);
    CHECK(strstr(buf, "cSON_apply_inline(_son_root, 0, poo())") != NULL);
    CHECK(strstr(buf, "cSON_apply_inline(_son_root, 1, ares())") != NULL);
    CHECK(strstr(buf, "cSON_collapse_inlines(_son_root)") != NULL);
    CHECK(strstr(buf, "if (!_son_root) {") != NULL);
    CHECK(strstr(buf, son) != NULL);
    free(buf);
}

static void test_integration_evalpoint(void) {
    const char* son = write_fixture("e2e.son",
        "#define VAR dick\n"
        "{\n"
        "    \"important code result\": {\n"
        "        !!\"__c__\": \"char* poo(){static char _b[32]; for(int d=0; d<999; d++){if (d==998){snprintf(_b,sizeof _b,\\\"%d\\\",d); return _b;}} return \\\"0\\\";}\"\n"
        "    }\n"
        "    \"assembler result\": {\n"
        "        !!\"__asm__\": \"char* ares(){int v; __asm__ volatile (\\\"mov $42, %0\\\" : \\\"=r\\\"(v)); static char _b[16]; snprintf(_b,sizeof _b,\\\"%d\\\",v); return _b;}\"\n"
        "    }\n"
        "    \"a\": {\n"
        "        !\"if(owo == \"VAR\")\": {\n"
        "            \"text\": \"Hello owo!!\"\n"
        "        }\n"
        "        !\"if(1)\": {\n"
        "            \"poop.txt\": \"microsoft\"\n"
        "        }\n"
        "    }\n"
        "}\n");
    char ass[1024];
    snprintf(ass, sizeof ass, "%s/SONEVAL.ass", dir);
    CHECK(cSON_gen_evalpoint(ass, son) == 1);

    const char* driver_src =
        "#define son\xF0\x9F\x98\xAD\xF0\x9F\x98\xAD\xF0\x9F\x98\xAD\xF0\x9F\x98\xAD\n"
        "#include \"../src/cSON.h\"\n"
        "#include \"SONEVAL.ass\"\n"
        "#include <string.h>\n"
        "int main(void) {\n"
        "    char* owo = \"dick\";\n"
        "    cSON_Obj* tree = cSON_evalpoint();\n"
        "    const char* v = cSON_get(tree, \"important code result\");\n"
        "    const char* a = cSON_get(tree, \"assembler result\");\n"
        "    const cSON_Obj* aobj = cSON_find(tree, \"a\");\n"
        "    const char* p = aobj ? cSON_get(aobj, \"poop.txt\") : NULL;\n"
        "    return (v && strcmp(v, \"998\") == 0 && a && strcmp(a, \"42\") == 0 && p && strcmp(p, \"microsoft\") == 0) ? 0 : 1;\n"
        "}\n";
    const char* driver_path = write_fixture("driver.c", driver_src);
    char bin[1024];
    snprintf(bin, sizeof bin, "%s/driver", dir);
    char cmd[2048];
    snprintf(cmd, sizeof cmd, "cc -std=c2x -o %s %s 2>%s/driver.err", bin, driver_path, dir);
    if (system(cmd) != 0) {
        char errpath[1024];
        snprintf(errpath, sizeof errpath, "%s/driver.err", dir);
        char* err = read_file(errpath);
        fprintf(stderr, "e2e compile failed:\n%s\n", err ? err : "(no stderr)");
        free(err);
        CHECK(0);
    } else {
        snprintf(cmd, sizeof cmd, "%s", bin);
        CHECK(system(cmd) == 0);
    }
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
    test_inline_c();
    test_inline_asm();
    test_brace_same_line();
    test_brace_empty();
    test_brace_closing_newline();
    test_brace_blank_lines();
    test_brace_adjacent_siblings();
    test_crlf();
    test_apply_if_unfold();
    test_apply_if_delete();
    test_apply_if_out_of_range();
    test_if_siblings();
    test_if_siblings_apply();
    test_apply_inline_collapse();
    test_apply_inline_no_collapse();
    test_gen_evalpoint();
    test_integration_evalpoint();
    test_empty();
    test_missing();

    char cmd[1024];
    snprintf(cmd, sizeof cmd, "rm -rf %s", dir);
    system(cmd);

    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
