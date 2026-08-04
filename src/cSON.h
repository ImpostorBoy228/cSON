#ifndef CSON_H
#define CSON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CSON_OBJECT,
    CSON_STRING,
    CSON_IF,
    CSON_INLINE_C,
    CSON_INLINE_ASM
} cSON_Type;

typedef struct cSON_Obj {
    cSON_Type type;
    struct cSON_Obj* parent;
    struct cSON_Obj* child;
    struct cSON_Obj* next;
    char* key;
    char* value;
} cSON_Obj;

int cSON_parse(cSON_Obj** root, const char* path);

void cSON_free(cSON_Obj* root);

const cSON_Obj* cSON_find(const cSON_Obj* obj, const char* key);

const char* cSON_get(const cSON_Obj* obj, const char* key);

void cSON_dump(const cSON_Obj* root, int depth);

void cSON_apply_if(cSON_Obj* root, int index, int value);

void cSON_apply_inline(cSON_Obj* root, int index, const char* value);

cSON_Obj* cSON_collapse_inlines(cSON_Obj* root);

int cSON_gen_evalpoint(const char* out_path, const char* son_path);

#ifdef son😭😭😭😭

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct cSON_Macro {
    char* name;
    char* value;
} cSON_Macro;

static void cSON_read_file(const char* path, char** out){
    FILE* file=fopen(path,"r");
    if(file==NULL){*out=NULL;return;}
    size_t capacity=32;
    char* buffer=malloc(capacity);
    if(buffer==NULL){fclose(file);*out=NULL;return;}
    size_t length=0;
    int ch;
    while((ch=fgetc(file))!=EOF){
        if(length+1>=capacity){
            capacity+=32;
            char* new_buffer=realloc(buffer,capacity);
            if(new_buffer==NULL){free(buffer);fclose(file);*out=NULL;return;}
            buffer=new_buffer;
        }
        buffer[length++]=(char)ch;
        if(ch=='\n'){buffer[length]='\0';}
    }
    buffer[length]='\0';
    *out=buffer;
    fclose(file);
}

static char* cSON_strdup(const char* s){
    size_t n=strlen(s)+1;
    char* p=malloc(n);
    if(p) memcpy(p,s,n);
    return p;
}

static const char* cSON_skip_ws(const char* s){
    while(*s==' '||*s=='\t'||*s=='\n'||*s=='\r')s++;
    return s;
}

static int cSON_buf_ensure(char** out, size_t* cap, size_t need) {
    if (need < *cap) return 1;
    size_t nc = *cap ? *cap : 64;
    while (nc < need) nc <<= 1;
    char* p = realloc(*out, nc);
    if (!p) return 0;
    *out = p;
    *cap = nc;
    return 1;
}

static int cSON_read_string(const char* s,int i,char** out,size_t* cap){
    size_t n=0;i++;
    while(s[i]!='\0'){
        char c=s[i];
        if(c=='"'||c=='\n'){ if(!cSON_buf_ensure(out,cap,n+1)) break; (*out)[n]='\0'; return i; }
        if(c=='\\'&&s[i+1]!='\0'){
            if(!cSON_buf_ensure(out,cap,n+2)) break;
            i++;
            switch(s[i]){
                case 'n': (*out)[n++]='\n';break;
                case 't': (*out)[n++]='\t';break;
                case '"': (*out)[n++]='"';break;
                case '\\': (*out)[n++]='\\';break;
                default: (*out)[n++]=s[i];break;
            }
            i++;continue;
        }
        if(!cSON_buf_ensure(out,cap,n+2)) break;
        (*out)[n++]=c;i++;
    }
    if(!cSON_buf_ensure(out,cap,n+1)) return i;
    (*out)[n]='\0';return i;
}

static int cSON_read_if_expr(const char* s,int i,char** out,size_t* cap){
    size_t n=0;i++;
    while(s[i]!='\0'){
        if(s[i]=='"'&&*cSON_skip_ws(s+i+1)==':'){ if(!cSON_buf_ensure(out,cap,n+1)) break; (*out)[n]='\0'; return i; }
        if(s[i]=='\n'){ if(!cSON_buf_ensure(out,cap,n+1)) break; (*out)[n]='\0'; return i; }
        if(!cSON_buf_ensure(out,cap,n+2)) break;
        (*out)[n++]=s[i];i++;
    }
    if(!cSON_buf_ensure(out,cap,n+1)) return i;
    (*out)[n]='\0';return i;
}

static int cSON_read_inline(const char* s,int i,char** out,size_t* cap){
    size_t n=0;i++;
    if(s[i]=='"')i++;
    while(s[i]!='\0'){
        if(s[i]=='"'||s[i]=='\n'){ if(!cSON_buf_ensure(out,cap,n+1)) break; (*out)[n]='\0'; return i; }
        if(!cSON_buf_ensure(out,cap,n+2)) break;
        (*out)[n++]=s[i];i++;
    }
    if(!cSON_buf_ensure(out,cap,n+1)) return i;
    (*out)[n]='\0';return i;
}

static char* cSON_resolve(const char* s,const cSON_Macro* macros,size_t n){
    for(size_t i=0;i<n;i++)
        if(strcmp(s,macros[i].name)==0)return cSON_strdup(macros[i].value);
    return cSON_strdup(s);
}

static char* cSON_resolve_all(const char* s,const cSON_Macro* macros,size_t n){
    char* out=cSON_strdup(s);
    for(size_t i=0;i<n;i++){
        char* p;
        size_t mlen=strlen(macros[i].name);
        size_t vlen=strlen(macros[i].value);
        while((p=strstr(out,macros[i].name))!=NULL){
            size_t plen=p-out;
            size_t rest=strlen(out+plen+mlen);
            char* tmp=malloc(plen+vlen+rest+1);
            memcpy(tmp,out,plen);
            memcpy(tmp+plen,macros[i].value,vlen);
            memcpy(tmp+plen+vlen,out+plen+mlen,rest+1);
            free(out);
            out=tmp;
        }
    }
    return out;
}

static cSON_Obj* cSON_obj_new(void){
    return calloc(1,sizeof(cSON_Obj));
}

static void cSON_obj_add(cSON_Obj* parent,cSON_Obj* o){
    if(!parent->child)parent->child=o;
    else{
        cSON_Obj* t=parent->child;
        while(t->next)t=t->next;
        t->next=o;
    }
    o->parent=parent;
}

int cSON_parse(cSON_Obj** root, const char* path) {
    char* text=NULL;
    cSON_read_file(path,&text);
    if(!text) return 0;

    const int len = strlen(text);
    char* defbuf=NULL; size_t defcap=0; size_t defbufpos=0;
    char* varbuf=NULL; size_t varcap=0; size_t varbufpos=0;
    char* valbuf=NULL; size_t valcap=0; size_t valbufpos=0;
    cSON_Macro* macros=NULL; size_t macrocap=0; size_t macroc=0;

    cSON_Obj* root_obj=NULL;
    cSON_Obj* cur=NULL;
    cSON_Obj* ifopen=NULL;
    char* pending_key=NULL;
    uint8_t pending_kind=0;

    for (int q=0;q<len;++q) {
        switch(text[q]){
            case ';':
                while (q < len) {
                    if (text[q] == '\n')
                        break;
                    q++;
                }
            break;
            case '#': {
                int p = 0;
                while (p < len) {
                    if (!cSON_buf_ensure(&defbuf, &defcap, defbufpos + 2)) goto oom;
                    defbuf[defbufpos++] = text[q+p];
                    if (text[q+p] == '\n') {
                        break;
                    }
                    p++;
                }
                defbuf[defbufpos] = '\0';
                p=7;
                while (defbuf[p] == ' ') p++;
                while ((size_t)p<defbufpos) {
                    if (defbuf[p]==' ') break;
                    if (!cSON_buf_ensure(&varbuf, &varcap, varbufpos + 2)) goto oom;
                    varbuf[varbufpos++] = defbuf[p];
                    p++;
                }
                p++;
                while ((size_t)p<defbufpos) {
                    if (defbuf[p]=='\n') break;
                    if (!cSON_buf_ensure(&valbuf, &valcap, valbufpos + 2)) goto oom;
                    valbuf[valbufpos++] = defbuf[p];
                    p++;
                }
                varbuf[varbufpos] = '\0';
                valbuf[valbufpos] = '\0';
                if (strncmp(defbuf, "#define", 7)==0) {
                    if (!cSON_buf_ensure((char**)&macros, &macrocap, (macroc + 1) * sizeof *macros)) goto oom;
                    macros[macroc].name = cSON_strdup(varbuf);
                    macros[macroc].value = cSON_strdup(valbuf);
                    macroc++;
                }
                varbufpos = 0;
                valbufpos = 0;
                q+=p;
                defbufpos = 0;
                break;
            }
            case '{': {
                cSON_Obj* o = cSON_obj_new();
                o->type = CSON_OBJECT;
                if (root_obj == NULL) {
                    root_obj = o;
                    cur = o;
                } else if (ifopen) {
                    if (cur) cSON_obj_add(ifopen, o);
                    ifopen = NULL;
                    cur = o;
                } else {
                    o->key = pending_key;
                    pending_key = NULL;
                    if (cur) cSON_obj_add(cur, o);
                    cur = o;
                }
                break;
            }
            case '}':
                ifopen = NULL;
                if (cur) {
                    cur = cur->parent;
                    if (cur && cur->type == CSON_IF) cur = cur->parent;
                }
                break;
            case '!': {
                if (text[q+1]=='!') {
                    char* buf = NULL; size_t cap = 0;
                    q = cSON_read_inline(text, q+1, &buf, &cap);
                    pending_kind = (strcmp(buf,"__asm__")==0) ? CSON_INLINE_ASM : CSON_INLINE_C;
                    free(buf);
                    break;
                }
                char* buf = NULL; size_t cap = 0;
                q = cSON_read_if_expr(text, q+1, &buf, &cap);
                cSON_Obj* o = cSON_obj_new();
                o->type = CSON_IF;
                o->key = pending_key;
                pending_key = NULL;
                o->value = cSON_resolve_all(buf, macros, macroc);
                if (cur) cSON_obj_add(cur, o);
                ifopen = o;
                free(buf);
                break;
            }
            case '"': {
                char* buf = NULL; size_t cap = 0;
                q = cSON_read_string(text, q, &buf, &cap);
                if (*cSON_skip_ws(text+q+1) == ':') {
                    if (pending_key) free(pending_key);
                    pending_key = cSON_strdup(buf);
                } else if (pending_key) {
                    cSON_Obj* o = cSON_obj_new();
                    o->type = CSON_STRING;
                    o->key = pending_key;
                    pending_key = NULL;
                    o->value = cSON_resolve(buf, macros, macroc);
                    if (cur) cSON_obj_add(cur, o);
                } else if (pending_kind) {
                    cSON_Obj* o = cSON_obj_new();
                    o->type = pending_kind;
                    pending_kind = 0;
                    o->value = cSON_resolve_all(buf, macros, macroc);
                    if (cur) cSON_obj_add(cur, o);
                } else {
                    fprintf(stderr, "your son is fucked\n");
                }
                free(buf);
                break;
            }
            default: break;
        }
    }

    *root = root_obj;
    free(pending_key);
    for(size_t i=0;i<macroc;i++){
        free(macros[i].name);
        free(macros[i].value);
    }
    free(macros);
    free(defbuf);
    free(varbuf);
    free(valbuf);
    free(text);
    return 1;

oom:
    cSON_free(root_obj);
    free(pending_key);
    for(size_t i=0;i<macroc;i++){
        free(macros[i].name);
        free(macros[i].value);
    }
    free(macros);
    free(defbuf);
    free(varbuf);
    free(valbuf);
    free(text);
    *root = NULL;
    return 0;
}

void cSON_free(cSON_Obj* o) {
    while(o){
        cSON_free(o->child);
        cSON_Obj* nx=o->next;
        free(o->key);
        free(o->value);
        free(o);
        o=nx;
    }
}

const cSON_Obj* cSON_find(const cSON_Obj* obj, const char* key) {
    if(!obj) return NULL;
    for(const cSON_Obj* c=obj->child;c;c=c->next)
        if(c->key && strcmp(c->key,key)==0) return c;
    return NULL;
}

const char* cSON_get(const cSON_Obj* obj, const char* key) {
    const cSON_Obj* c=cSON_find(obj,key);
    if(c && c->type != CSON_OBJECT) return c->value;
    return NULL;
}

void cSON_dump(const cSON_Obj* o, int d) {
    if(!o) return;
    for(const cSON_Obj* c=o->child;c;c=c->next){
        for(int i=0;i<d;i++)printf("  ");
        if(c->type == CSON_IF){
            printf("IF \"%s\" {\n",c->value?c->value:"?");
            if (c->child) cSON_dump(c->child, d+1);
            for(int i=0;i<d;i++)printf("  ");
            printf("}\n");
            continue;
        }
        if(c->type == CSON_INLINE_C || c->type == CSON_INLINE_ASM){
            printf("!!\"%s\": \"%s\"\n",
                c->type == CSON_INLINE_C ? "__c__" : "__asm__",
                c->value?c->value:"?");
            continue;
        }
        if(c->key)printf("%s: ",c->key);
        if(c->child){
            printf("{\n");
            cSON_dump(c,d+1);
            for(int i=0;i<d;i++)printf("  ");
            printf("}\n");
        } else{
            printf("\"%s\"\n",c->value?c->value:"?");
        }
    }
}

static cSON_Obj* cSON_if_by_index(cSON_Obj* o, int* counter, int target) {
    if (!o) return NULL;
    for (cSON_Obj* c = o->child; c; c = c->next) {
        if (c->type == CSON_IF) {
            if (*counter == target) return c;
            (*counter)++;
        }
        if (c->child) {
            cSON_Obj* f = cSON_if_by_index(c, counter, target);
            if (f) return f;
        }
    }
    return NULL;
}

void cSON_apply_if(cSON_Obj* root, int index, int value) {
    if (!root || index < 0) return;
    int counter = 0;
    cSON_Obj* c = cSON_if_by_index(root, &counter, index);
    if (!c) return;
    cSON_Obj* parent = c->parent;
    if (!parent) return;

    cSON_Obj* prev = NULL;
    for (cSON_Obj* p = parent->child; p && p != c; p = p->next) prev = p;
    if (prev) prev->next = c->next;
    else parent->child = c->next;

    if (value != 0) {
        cSON_Obj* body = c->child;
        if (body && body->child) {
            cSON_Obj* last = body->child;
            while (last->next) last = last->next;
            if (prev) prev->next = body->child;
            else parent->child = body->child;
            last->next = c->next;
            for (cSON_Obj* b = body->child; b; b = b->next) b->parent = parent;
            body->child = NULL;
        }
        cSON_free(body);
        c->child = NULL;
        c->next = NULL;
        cSON_free(c);
    } else {
        c->next = NULL;
        cSON_free(c);
    }
}

static cSON_Obj* cSON_inline_by_index(cSON_Obj* o, int* counter, int target) {
    if (!o) return NULL;
    for (cSON_Obj* c = o->child; c; c = c->next) {
        if (c->type == CSON_INLINE_C || c->type == CSON_INLINE_ASM) {
            if (*counter == target) return c;
            (*counter)++;
        }
        if (c->child) {
            cSON_Obj* f = cSON_inline_by_index(c, counter, target);
            if (f) return f;
        }
    }
    return NULL;
}

void cSON_apply_inline(cSON_Obj* root, int index, const char* value) {
    if (!root || index < 0 || !value) return;
    int counter = 0;
    cSON_Obj* c = cSON_inline_by_index(root, &counter, index);
    if (!c) return;
    free(c->value);
    c->value = cSON_strdup(value);
}

typedef struct {
    int index;
    char* value;
} cSON_IFPair;

static void cSON_collect_ifs(cSON_Obj* o, cSON_IFPair** out, int* count, int* cap) {
    for (cSON_Obj* c = o->child; c; c = c->next) {
        if (c->type == CSON_IF) {
            if (*count == *cap) {
                *cap = *cap ? *cap * 2 : 16;
                *out = realloc(*out, (size_t)*cap * sizeof **out);
            }
            (*out)[*count].index = *count;
            (*out)[*count].value = c->value ? cSON_strdup(c->value) : NULL;
            (*count)++;
        }
        if (c->child) cSON_collect_ifs(c, out, count, cap);
    }
}

typedef struct {
    int index;
    int kind;
    char* value;
} cSON_InlinePair;

static void cSON_collect_inlines(cSON_Obj* o, cSON_InlinePair** out, int* count, int* cap) {
    for (cSON_Obj* c = o->child; c; c = c->next) {
        if (c->type == CSON_INLINE_C || c->type == CSON_INLINE_ASM) {
            if (*count == *cap) {
                *cap = *cap ? *cap * 2 : 16;
                *out = realloc(*out, (size_t)*cap * sizeof **out);
            }
            (*out)[*count].index = *count;
            (*out)[*count].kind = c->type;
            (*out)[*count].value = c->value ? cSON_strdup(c->value) : NULL;
            (*count)++;
        }
        if (c->child) cSON_collect_inlines(c, out, count, cap);
    }
}

static int cSON_has_inline(cSON_Obj* o) {
    if (!o) return 0;
    for (cSON_Obj* c = o->child; c; c = c->next) {
        if (c->type == CSON_INLINE_C || c->type == CSON_INLINE_ASM) return 1;
        if (c->child && cSON_has_inline(c)) return 1;
    }
    return 0;
}

static void cSON_fn_name(const char* code, char* out, size_t cap) {
    if (!code || cap == 0) { if (cap) out[0] = '\0'; return; }
    const char* p = strchr(code, '(');
    if (!p) { out[0] = '\0'; return; }
    const char* q = p - 1;
    while (q >= code && (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r' || *q == '*')) q--;
    const char* end = q + 1;
    while (q >= code && ((*q >= 'a' && *q <= 'z') || (*q >= 'A' && *q <= 'Z') ||
                         (*q >= '0' && *q <= '9') || *q == '_')) q--;
    const char* start = q + 1;
    size_t n = (size_t)(end - start);
    if (n == 0 || n >= cap) { out[0] = '\0'; return; }
    memcpy(out, start, n);
    out[n] = '\0';
}

static cSON_Obj* cSON_collapse_list(cSON_Obj* o) {
    for (cSON_Obj* c = o; c; c = c->next) {
        if (c->child) c->child = cSON_collapse_list(c->child);
        if (c->type == CSON_OBJECT && c->child && !c->child->next &&
            (c->child->type == CSON_INLINE_C || c->child->type == CSON_INLINE_ASM)) {
            cSON_Obj* inl = c->child;
            free(c->value);
            c->value = inl->value;
            inl->value = NULL;
            c->type = CSON_STRING;
            c->child = NULL;
            cSON_free(inl);
        }
    }
    return o;
}

cSON_Obj* cSON_collapse_inlines(cSON_Obj* root) {
    if (!root) return NULL;
    root->child = cSON_collapse_list(root->child);
    if (root->type == CSON_OBJECT && root->child && !root->child->next &&
        (root->child->type == CSON_INLINE_C || root->child->type == CSON_INLINE_ASM)) {
        cSON_Obj* inl = root->child;
        free(root->value);
        root->value = inl->value;
        inl->value = NULL;
        root->type = CSON_STRING;
        root->child = NULL;
        cSON_free(inl);
    }
    return root;
}

static void cSON_escape_cstr(const char* in, char* out, size_t cap) {
    size_t n = 0;
    for (const char* p = in; *p && n + 2 < cap; p++) {
        if (*p == '"') { out[n++] = '\\'; out[n++] = '"'; }
        else if (*p == '\\') { out[n++] = '\\'; out[n++] = '\\'; }
        else out[n++] = *p;
    }
    out[n] = '\0';
}

int cSON_gen_evalpoint(const char* out_path, const char* son_path) {
    if (!out_path || !son_path) return 0;
    cSON_Obj* root = NULL;
    if (!cSON_parse(&root, son_path) || !root) return 0;

    cSON_IFPair* pairs = NULL;
    int count = 0, cap = 0;
    cSON_collect_ifs(root, &pairs, &count, &cap);

    cSON_InlinePair* ipairs = NULL;
    int icount = 0, icap = 0;
    cSON_collect_inlines(root, &ipairs, &icount, &icap);
    int has_inl = cSON_has_inline(root);

    char son[1024];
    cSON_escape_cstr(son_path, son, sizeof son);

    FILE* f = fopen(out_path, "w");
    if (!f) {
        for (int i = 0; i < count; i++) free(pairs[i].value);
        free(pairs);
        for (int i = 0; i < icount; i++) free(ipairs[i].value);
        free(ipairs);
        cSON_free(root);
        return 0;
    }

    fprintf(f, "#pragma once\n\n");

    if (has_inl) {
        for (int i = 0; i < icount; i++) {
            if (ipairs[i].value)
                fprintf(f, "static %s\n\n", ipairs[i].value);
        }
    }

    fprintf(f, "#define cSON_evalpoint() ({ \\\n");
    fprintf(f, "    static cSON_Obj* _son_root = NULL; \\\n");
    fprintf(f, "    if (!_son_root) { \\\n");
    fprintf(f, "        cSON_parse(&_son_root, \"%s\"); \\\n", son);
    for (int i = count - 1; i >= 0; i--) {
        const char* cond = pairs[i].value ? pairs[i].value : "0";
        fprintf(f, "        %s { cSON_apply_if(_son_root, %d, 1); } else { cSON_apply_if(_son_root, %d, 0); } \\\n",
                cond, pairs[i].index, pairs[i].index);
    }
    if (has_inl) {
        for (int i = 0; i < icount; i++) {
            if (ipairs[i].value) {
                char name[128];
                cSON_fn_name(ipairs[i].value, name, sizeof name);
                fprintf(f, "        cSON_apply_inline(_son_root, %d, %s()); \\\n", ipairs[i].index, name);
            }
        }
    }
    fprintf(f, "        _son_root = cSON_collapse_inlines(_son_root); \\\n");
    fprintf(f, "    } \\\n");
    fprintf(f, "    _son_root; \\\n");
    fprintf(f, "})\n");

    fclose(f);
    for (int i = 0; i < count; i++) free(pairs[i].value);
    free(pairs);
    for (int i = 0; i < icount; i++) free(ipairs[i].value);
    free(ipairs);
    cSON_free(root);
    return 1;
}

#endif /* son😭😭😭😭 */

#ifdef __cplusplus
}
#endif

#endif /* CSON_H */
