#define son😭😭😭😭
#include "cSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CSON_NO_EVALPOINT
#include "../SONEVAL.ass"
#endif

int main(int argc, char* argv[]) {
    if (argc >= 4 && strcmp(argv[1], "--gen-evalpoint") == 0) {
        return cSON_gen_evalpoint(argv[2], argv[3]) ? 0 : 1;
    }
#ifndef CSON_NO_EVALPOINT
    char* owo = "dick";
    cSON_Obj* tree = cSON_evalpoint();
    cSON_dump(tree, 0);
    cSON_free(tree);
#endif
    return 0;
}
