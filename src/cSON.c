#define CSON_IMPLEMENTATION
#include "cSON.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    cSON_Obj* root = NULL;
    if (cSON_parse(&root, argc > 1 ? argv[1] : "emmm.son")) {
        fprintf(stderr, "FUCK!\n");
        return 1;
    }
    cSON_dump(root, 0);
    cSON_free(root);
    return 0;
naxyi:
}
