#define son😭😭😭😭
#include "cSON.h"
#include "SONEVAL.ass"
#include <stdio.h>

int main(void) {
    char* owo = "dick";
    cSON_Obj* tree = cSON_evalpoint();
    const cSON_Obj* emm = cSON_find(tree, "emm");
    printf("%s\n", cSON_get(emm, "f1"));                      // lol
    printf("%s\n", cSON_get(emm, "important code result"));  // 998
    printf("%s\n", cSON_get(emm, "assembler result"));       // 42
    cSON_free(tree);
    return 0;
}
