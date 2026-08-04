#define son😭😭😭😭
#include "cSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../SONEVAL.ass"

int main(int argc, char* argv[]) {
    if (argc >= 4 && strcmp(argv[1], "--gen-evalpoint") == 0) {
        return cSON_gen_evalpoint(argv[2], argv[3]) ? 0 : 1;
    }
    char* owo = "dick";
    cSON_Obj* tree = cSON_evalpoint();
    cSON_dump(tree, 0);
    cSON_free(tree);
    return 0;
}
