## son😭😭😭😭
### a great json alternative. Why son? json was father😭😭😭

### contents (click the shit)
- [features](#features)
- [full .son file example](#full-son-file-example)
- [how this shit compiles](#how-this-shit-compiles-and-how-it-supports-if-expressions-and-executions-inside-c)
- [what is `cSON_evalpoint()`?](#what-is-csonevalpoint)
- [how to use this shit](#how-to-use-this-shit)
- [api docs](#api-docs)

### features
1. only objects and strings. genius in minimalism.
2. written in C like chads do
3. one line comments in assembly style( ; comment )
4. ***if*** statements evaluating directly in c code. to create a new one, add value assigned `!"if(_expression_)"` and key is object with KV pairs supposed to shown if `_expression_` is true
Example:
```son
!"if(a==3)": {"poop.txt": "microsoft"}
```
4. inline c. open new object as value with one KV pair where key is assigned `!!"__c__"` Example:
```son
"code_result": { !!"__c__": "char* poo(){static char _b[32]; for(int d=0; d<999; d++){if (d==998){snprintf(_b,sizeof _b,\"%d\",d); return _b;}} return \"0\";}" }
```
It is important to write code in `char* function()`

5. #define macros(stupid text replace) Note: *dont works in inline c and ifs*
```son
#define VAR VAL
```
Only this syntax supported. one define at one string. no comments at this string.

6. vim syntax highlighting. install:
   `mkdir -p ~/.vim/syntax && cp son.vim ~/.vim/syntax/son.vim`, then in `~/.vimrc`: `autocmd BufNewFile,BufRead *.son setfiletype son`

### full .son file example
```son
;comment
i am asshole ;<- should ignore
#define VAR dick
{
    "emm": {
        "f1": "lol"
        "f2": "bubububu"
	"important code result": { !!"__c__": "char* poo(){static char _b[32]; for(int d=0; d<999; d++){if (d==998){snprintf(_b,sizeof _b,\"%d\",d); return _b;}} return \"0\";}" }
"assembler result"      :   { !!"__asm__": "char* ares(){int v; __asm__ volatile (\"mov $42, %0\" : \"=r\"(v)); static char _b[16]; snprintf(_b,sizeof _b,\"%d\",v); return _b;}" } ; shitty indentations are ok👌👌
        "cock": "VAR" ; smart comment
    }
    "a": {
        !"if(owo == "VAR")": {
            "text": "Hello owo!!"
        }
        !"if(1)": {"poop.txt": "microsoft"}
    }
    fuck microsft ;<- should ignore
}
```

### How this shit compiles and how it supports if expressions and executions inside C
`cSON_gen_evalpoint()` is "compiling" .son file(2nd argument) to file with custom name(1st argument). The example above compiles into this:
```C
#pragma once

static char* poo(){static char _b[32]; for(int d=0; d<999; d++){if (d==998){snprintf(_b,sizeof _b,"%d",d); return _b;}} return "0";}

static char* ares(){int v; __asm__ volatile ("mov $42, %0" : "=r"(v)); static char _b[16]; snprintf(_b,sizeof _b,"%d",v); return _b;}

#define cSON_evalpoint() ({ \
    static cSON_Obj* _son_root = NULL; \
    if (!_son_root) { \
        cSON_parse(&_son_root, "emmm.son"); \
        if(1) { cSON_apply_if(_son_root, 1, 1); } else { cSON_apply_if(_son_root, 1, 0); } \
        if(owo == "dick") { cSON_apply_if(_son_root, 0, 1); } else { cSON_apply_if(_son_root, 0, 0); } \
        cSON_apply_inline(_son_root, 0, poo()); \
        cSON_apply_inline(_son_root, 1, ares()); \
        _son_root = cSON_collapse_inlines(_son_root); \
    } \
    _son_root; \
})
```
Since the .son will not change from the moment of compilation, we can inject if expressions directly in c, when compile the c code.

##### What is `cSON_evalpoint()`?
It is *macro-function* what used to evaluate *ifs* and inline c code at the moment of execution. Since it is macro, we can access all variables, etc.

### How to use this shit
1. define this file is implementation `#define son😭😭😭😭` (only in ONE file, in others just `#include "cSON.h"`)
2. include cSON lib `#include "cSON.h"`
3. use cSON_gen_evalpoint() or use SonCompiler
4. `#include "SONEVAL.ass"` where you want `cSON_evalpoint()`
5. call `cSON_evalpoint()`, read values with `cSON_get`/`cSON_find`, and `cSON_free()` when done or leak like a sieve

example:
```c
#define son😭😭😭😭
#include "cSON.h"
#include "SONEVAL.ass"
#include <stdio.h>

int main(void) {
    char* owo = "dick";
    cSON_Obj* tree = cSON_evalpoint();
    const cSON_Obj* emm = cSON_find(tree, "emm");
    printf("%s\n", cSON_get(emm, "f1"));                      // lol
    printf("%s\n", cSON_get(tree, "important code result"));  // 998
    printf("%s\n", cSON_get(tree, "assembler result"));       // 42
    cSON_free(tree);
    return 0;
}
```
(free the tree, not the vibe)

### API docs
everything takes a `cSON_Obj*` tree. root is an object, its `child` list holds objects or strings.

- `int cSON_parse(cSON_Obj** root, const char* path)` — parse `.son` file into tree. returns 1 if ok, 0 if file missing or out of memory. empty file = 1 but `*root` is NULL
- `void cSON_free(cSON_Obj* root)` — free the whole tree
- `const cSON_Obj* cSON_find(const cSON_Obj* obj, const char* key)` — find direct child by key, or NULL. go deeper with nested `cSON_find` or `->child`/`->next`
- `const char* cSON_get(const cSON_Obj* obj, const char* key)` — get string value of direct child, or NULL if missing or if the child is an object (then use `cSON_find`)
- `void cSON_dump(const cSON_Obj* root, int depth)` — print the tree to stdout. pass 0 as depth
- `void cSON_apply_if(cSON_Obj* root, int index, int value)` — evaluate if #`index` (counted in order of appearance). `value != 0` unfolds the body into parent, `0` deletes it. called inside `cSON_evalpoint`
- `void cSON_apply_inline(cSON_Obj* root, int index, const char* value)` — replace the result of inline c/asm #`index` with `value`. called inside `cSON_evalpoint`
- `cSON_Obj* cSON_collapse_inlines(cSON_Obj* root)` — turn objects with a single inline child into plain strings (`"res": {!!"__c__": ...}` -> `"res": "998"`). returns root. called inside `cSON_evalpoint`
- `int cSON_gen_evalpoint(const char* out_path, const char* son_path)` — generate `SONEVAL.ass` (static funcs + `cSON_evalpoint` macro). returns 1 if ok
- `cSON_Obj` — a node: `type`, `parent`, `child`, `next`, `key`, `value`
- `cSON_Type` — `CSON_OBJECT`, `CSON_STRING`, `CSON_IF`, `CSON_INLINE_C`, `CSON_INLINE_ASM`
