#define son😭😭😭😭
#include "cSON.h"

#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <file.son> <output>\n", argv[0]);
        return 1;
    }
    return cSON_gen_evalpoint(argv[2], argv[1]) ? 0 : 1;
}
