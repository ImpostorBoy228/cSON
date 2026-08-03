#ifndef CSON_H
#define CSON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CSON_OBJECT,
    CSON_STRING,
    CSON_IF
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

#ifdef CSON_IMPLEMENTATION

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

static char* cSON_strdup(const char *s){
    size_t n=strlen(s)+1;
    char* p=malloc(n);
    if(p) memcpy(p,s,n);
    return p;
}

static const char* cSON_skip_ws(const char* s){
    while(*s==' '||*s=='\t'||*s=='\n'||*s=='\r')s++;
    return s;
}

static int cSON_read_string(const char* s,int i,char* out,size_t cap){
    size_t n=0;i++;
    while(s[i]!='\0'&&n+1<cap){
        char c=s[i];
        if(c=='"'||c=='\n'){out[n]='\0';return i;}
        if(c=='\\'&&s[i+1]!='\0'){
            i++;
            switch(s[i]){
                case 'n': out[n++]='\n';break;
                case 't': out[n++]='\t';break;
                case '"': out[n++]='"';break;
                case '\\': out[n++]='\\';break;
                default: out[n++]=s[i];break;
            }
            i++;continue;
        }
        out[n++]=c;i++;
    }
    out[n]='\0';return i;
}

static int cSON_read_if_expr(const char* s,int i,char* out,size_t cap){
    size_t n=0;i++;
    while(s[i]!='\0'&&n+1<cap){
        if(s[i]=='"'&&*cSON_skip_ws(s+i+1)==':'){out[n]='\0';return i;}
        if(s[i]=='\n'){out[n]='\0';return i;}
        out[n++]=s[i];i++;
    }
    out[n]='\0';return i;
}

static char* cSON_resolve(const char* s,const cSON_Macro* macros,uint16_t n){
    for(uint16_t i=0;i<n;i++)
        if(strcmp(s,macros[i].name)==0)return cSON_strdup(macros[i].value);
    return cSON_strdup(s);
}

static char* cSON_resolve_all(const char* s,const cSON_Macro* macros,uint16_t n){
    char* out=cSON_strdup(s);
    for(uint16_t i=0;i<n;i++){
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
    if(!text) return 1;

    const int len = strlen(text);
    char defbuf[1024];
    char varbuf[64];
    char valbuf[127];
    uint16_t defbufpos=0;
    uint8_t varbufpos=0;
    uint8_t valbufpos=0;
    cSON_Macro macros[1024];
    uint16_t macroc=0;

    cSON_Obj* root_obj=NULL;
    cSON_Obj* cur=NULL;
    cSON_Obj* ifopen=NULL;
    char* pending_key=NULL;

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
                    defbuf[defbufpos++] = text[q+p];
                    if (text[q+p] == '\n') {
                        break;
                    }
                    p++;
                }
                defbuf[defbufpos] = '\0';
                p=7;
                while (defbuf[p] == ' ') p++;
                while (p<defbufpos) {
                    if (defbuf[p]==' ') break;
                    varbuf[varbufpos++] = defbuf[p];
                    p++;
                }
                p++;
                while (p<defbufpos) {
                    if (defbuf[p]=='\n') break;
                    valbuf[valbufpos++] = defbuf[p];
                    p++;
                }
                varbuf[varbufpos] = '\0';
                valbuf[valbufpos] = '\0';
                if (strncmp(defbuf, "#define", 7)==0) {
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
                if (cur) cur = cur->parent;
                break;
            case '!': {
                char buf[512];
                q = cSON_read_if_expr(text, q+1, buf, sizeof buf);
                cSON_Obj* o = cSON_obj_new();
                o->type = CSON_IF;
                o->key = pending_key;
                pending_key = NULL;
                o->value = cSON_resolve_all(buf, macros, macroc);
                if (cur) cSON_obj_add(cur, o);
                ifopen = o;
                break;
            }
            case '"': {
                char buf[512];
                q = cSON_read_string(text, q, buf, sizeof buf);
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
                } else {
                    fprintf(stderr, "your son is fucked\n");
                }
                break;
            }
            case ':':
            case ',':
            case '\\':
            default: break;
        }
    }

    *root = root_obj;
    free(pending_key);
    for(uint16_t i=0;i<macroc;i++){
        free(macros[i].name);
        free(macros[i].value);
    }
    free(text);
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
        if(c->key)printf("%s: ",c->key);
        if(c->child){
            printf("{\n");
            cSON_dump(c,d+1);
            for(int i=0;i<d;i++)printf("  ");
            printf("}\n");
        }else{
            printf("\"%s\"\n",c->value?c->value:"?");
        }
    }
}

#endif /* CSON_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

#endif /* CSON_H */
