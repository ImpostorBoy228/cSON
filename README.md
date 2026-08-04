## son😭😭😭😭
### a great json alternative. Why son? json was father😭😭😭
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
1. define this file is implementation `#define son😭😭😭😭`
2. include cSON lib `#include "cSON.h"`
3. _to be continued_

### API docs
_here api docs_
