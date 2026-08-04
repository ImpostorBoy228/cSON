# OPLAN — inline C/asm eval (p.1)

## Конвенция

`!!"__c__"` value = полное определение C-функции. Генератор извлекает имя
функции (токен перед первой `(`), вставляет код дословно в `SONEVAL.ass`
как функцию на уровне файла, макрос `cSON_evalpoint()` вызывает `<имя>()`
и пишет результат (стрингифицированный через `%lld`) в inline-ноду.
Затем `cSON_collapse_inlines()` схлопывает объект с единственной inline-нодой
в value-ноду, и `cSON_get(root, "important code result")` возвращает `"998"`.

Пример `emmm.son` остаётся без правок:
```
"important code result": { !!"__c__": "int poo(){for(int d=0; d<999; d++){if (d==998) return d;}return 0;}" }
```

## Изменения

### src/cSON.h

- **Новые публичные функции** (рядом с `cSON_apply_if`):
  - `void cSON_apply_inline(cSON_Obj* root, int index, const char* value)` —
    по аналогии с `cSON_apply_if`: DFS по inline-нодам, замена `value`.
  - `cSON_Obj* cSON_collapse_inlines(cSON_Obj* root)` — объект с единственным
    inline-ребёнком превращается в value-ноду (`CSON_STRING`, ключ сохраняется);
    корневой edge-case возвращает новый root.
  - хелперы: `cSON_collect_inlines`, `cSON_has_inline`, `cSON_fn_name`
    (извлечение имени функции из кода).
- **`cSON_gen_evalpoint`**:
  - собрать inline-ноды (index, kind, код) — порядок DFS как у `cSON_collect_ifs`.
  - **фикс IF-квирка**: parse + IF-применения + inline + collapse завернуть в
    `if (!_son_root) { ... }` (повторные вызовы `cSON_evalpoint()` безопасны).
  - перед `#define cSON_evalpoint()` эмитить функции inline-кода дословно.
  - в макросе после IF-цикла, если есть inline:
    ```c
    char _r0[64];
    snprintf(_r0, sizeof _r0, "%lld", (long long)poo());
    cSON_apply_inline(_son_root, 0, _r0);
    ```
  - в конце: `_son_root = cSON_collapse_inlines(_son_root);`.

Сгенерированный `SONEVAL.ass`:
```c
#ifndef SONEVAL_ASS
#define SONEVAL_ASS

int poo(){for(int d=0; d<999; d++){if (d==998) return d;}return 0;}

#define cSON_evalpoint() ({ \
    static cSON_Obj* _son_root = NULL; \
    if (!_son_root) { \
        cSON_parse(&_son_root, "emmm.son"); \
        if(owo == "dick") { cSON_apply_if(_son_root, 0, 1); } else { cSON_apply_if(_son_root, 0, 0); } \
        char _r0[64]; \
        snprintf(_r0, sizeof _r0, "%lld", (long long)poo()); \
        cSON_apply_inline(_son_root, 0, _r0); \
        _son_root = cSON_collapse_inlines(_son_root); \
    } \
    _son_root; \
})
#endif
```

### tests/test.c

- unit: `cSON_apply_inline` + collapse → `cSON_get == "998"`;
  негативка — объект с двумя детьми не схлопывается.
- `test_gen_evalpoint`: assert в файле есть `int poo(){...}`,
  `(long long)poo()`, `cSON_apply_inline`, `cSON_collapse_inlines`,
  guard `if (!_son_root) {`.
- **интеграционный**: фикстура → генерация evalpoint → драйвер-`main`
  с `#include "SONEVAL.ass"`, вызов `cSON_evalpoint()`,
  `strcmp(cSON_get(...), "998")` → компиляция через `cc` + запуск (end-to-end).

### Не трогаем

`emmm.son`, `cSON_dump`, `Makefile`.

## Ограничения p.1

- имя функции извлекается эвристикой (простой случай `int name(...)`)
- результат кастуется в `long long` (`%lld`) — не `void`/`struct`/`double`
- имена функций должны быть уникальны
- `!!"__asm__"` пока вставляется как statement без value
