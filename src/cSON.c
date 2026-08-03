#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define CSON_OBJECT    1
#define CSON_ARRAY     2
#define CSON_STRING    3
#define CSON_NUMBER    4
#define CSON_PRIMITIVE 5

typedef struct cSON_Item {
    int type;
    char* key;
    char* valuestring;
    double valuedouble;
    struct cSON_Item* child;
    struct cSON_Item* next;
    struct cSON_Item* prev;
} cSON_Item;

typedef struct Macro { char* name; char* value; } Macro;

#define TOK_OBJ_OPEN  1
#define TOK_OBJ_CLOSE 2
#define TOK_ARR_OPEN  3
#define TOK_ARR_CLOSE 4
#define TOK_STRING    5
#define TOK_PRIMITIVE 6
#define TOK_EXPR      7
#define TOK_COLON     8
#define TOK_COMMA     9

typedef struct { int type; int start; int end; } Token;

typedef struct Frame {
    cSON_Item* cont;
    cSON_Item* tail;
    char* pending_key;
} Frame;

#define MAXTOK 4096
#define MAXFRAME 256

void fileread(const char* filename, char** out){FILE* file=fopen(filename,"r");if(file==NULL){*out=NULL;return;}size_t capacity=32;char*buffer=malloc(capacity);if(buffer==NULL){fclose(file);*out=NULL;return;}size_t length=0;int ch;while((ch=fgetc(file))!=EOF){if(length+1>=capacity){capacity+=32;char*new_buffer=realloc(buffer,capacity);if(new_buffer==NULL){free(buffer);fclose(file);*out=NULL;return;}buffer=new_buffer;}buffer[length++]=(char)ch;}buffer[length]='\0';*out=buffer;fclose(file);}
char* substr(const char *s, size_t begin, size_t end){size_t len=end-begin+1;char* out=malloc(len+1);if(!out)return NULL;memcpy(out,s+begin,len);out[len]='\0';return out;}
char* xstrdup(const char *s){size_t n=strlen(s)+1;char* p=malloc(n);if(p) memcpy(p,s,n);return p;}

static char* replace_all(const char* src, const char* from, const char* to) {
    size_t fromlen = strlen(from);
    size_t tolen = strlen(to);
    if (fromlen == 0) return xstrdup(src);
    size_t count = 0;
    const char* p = src;
    while ((p = strstr(p, from)) != NULL) { count++; p += fromlen; }
    long delta = (long)tolen - (long)fromlen;
    size_t outlen = (size_t)((long)strlen(src) + (long)count * delta) + 1;
    char* out = malloc(outlen);
    if (!out) return NULL;
    char* w = out;
    p = src;
    while (count--) {
        const char* m = strstr(p, from);
        size_t pre = (size_t)(m - p);
        memcpy(w, p, pre); w += pre;
        memcpy(w, to, tolen); w += tolen;
        p = m + fromlen;
    }
    strcpy(w, p);
    return out;
}

static cSON_Item* cSON_New(int type) {
    cSON_Item* it = calloc(1, sizeof(cSON_Item));
    if (it) it->type = type;
    return it;
}

static void cSON_Delete(cSON_Item* item) {
    cSON_Item* next;
    while (item) {
        next = item->next;
        if (item->child) cSON_Delete(item->child);
        free(item->key);
        free(item->valuestring);
        free(item);
        item = next;
    }
}

static void attach_value(Frame* f, cSON_Item* item) {
    item->key = f->pending_key;
    f->pending_key = NULL;
    if (!f->tail) f->cont->child = item;
    else { f->tail->next = item; item->prev = f->tail; }
    f->tail = item;
}

static void tok_push(Token* toks, int* tokc, int type, int start, int end) {
    if (*tokc >= MAXTOK) return;
    toks[*tokc].type = type;
    toks[*tokc].start = start;
    toks[*tokc].end = end;
    (*tokc)++;
}

static void print_item(cSON_Item* it, int depth) {
    if (!it) return;
    for (int i = 0; i < depth; i++) printf("  ");
    switch (it->type) {
    case CSON_OBJECT:    printf("OBJECT");    break;
    case CSON_ARRAY:     printf("ARRAY");     break;
    case CSON_STRING:    printf("STRING");    break;
    case CSON_NUMBER:    printf("NUMBER");    break;
    default:             printf("PRIMITIVE"); break;
    }
    if (it->key) printf(" key=\"%s\"", it->key);
    if (it->valuestring) printf(" val=\"%s\"", it->valuestring);
    if (it->type == CSON_NUMBER) printf(" val=%g", it->valuedouble);
    printf("\n");
    for (cSON_Item* ch = it->child; ch; ch = ch->next) print_item(ch, depth + 1);
}

static cSON_Item* build_tree(Token* toks, int tokc, const char* src) {
    Frame stack[MAXFRAME];
    int sp = 0;
    cSON_Item* root = NULL;
    for (int i = 0; i < tokc; i++) {
        Token t = toks[i];
        switch (t.type) {
        case TOK_OBJ_OPEN:
        case TOK_ARR_OPEN: {
            cSON_Item* item = cSON_New(t.type == TOK_OBJ_OPEN ? CSON_OBJECT : CSON_ARRAY);
            if (sp == 0) root = item;
            else attach_value(&stack[sp - 1], item);
            if (sp < MAXFRAME) {
                stack[sp].cont = item;
                stack[sp].tail = NULL;
                stack[sp].pending_key = NULL;
                sp++;
            }
            break;
        }
        case TOK_OBJ_CLOSE:
        case TOK_ARR_CLOSE:
            if (sp > 0) sp--;
            break;
        case TOK_STRING: {
            char* text = substr(src, t.start, t.end - 1);
            if (sp == 0) { free(text); break; }
            Frame* top = &stack[sp - 1];
            if (top->cont->type == CSON_OBJECT && top->pending_key == NULL) {
                top->pending_key = text;
            } else {
                cSON_Item* item = cSON_New(CSON_STRING);
                item->valuestring = text;
                attach_value(top, item);
            }
            break;
        }
        case TOK_PRIMITIVE: {
            char* text = substr(src, t.start, t.end - 1);
            if (sp == 0) { free(text); break; }
            Frame* top = &stack[sp - 1];
            if (top->cont->type == CSON_OBJECT && top->pending_key == NULL) {
                free(text);
                break;
            }
            cSON_Item* item = cSON_New(CSON_PRIMITIVE);
            char* endp = NULL;
            double d = strtod(text, &endp);
            if (endp != text && *endp == '\0') {
                item->type = CSON_NUMBER;
                item->valuedouble = d;
                free(text);
            } else {
                item->valuestring = text;
            }
            attach_value(top, item);
            break;
        }
        case TOK_EXPR: {
            char* text = substr(src, t.start, t.end - 1);
            if (sp == 0) { free(text); break; }
            Frame* top = &stack[sp - 1];
            if (top->cont->type == CSON_OBJECT && top->pending_key == NULL) {
                top->pending_key = text;
            } else {
                cSON_Item* item = cSON_New(CSON_PRIMITIVE);
                item->valuestring = text;
                attach_value(top, item);
            }
            break;
        }
        case TOK_COLON:
        case TOK_COMMA:
            break;
        }
    }
    return root;
}

int main(int argc, char** argv) {
    const char* filename = (argc > 1) ? argv[1] : "emmm.son";
    char* sonfile;
    fileread(filename, &sonfile);
    if (!sonfile) goto naxyi;
    int len = strlen(sonfile);

    char defbuf[1024];
    char varbuf[64];
    char valbuf[127];
    uint16_t defbufpos = 0;
    uint8_t varbufpos = 0;
    uint8_t valbufpos = 0;
    Macro macros[1024];
    uint16_t macroc = 0;

    for (int q = 0; q < len; ++q) {
        switch (sonfile[q]) {
        case ';':
            while (q < len && sonfile[q] != '\n') q++;
            break;
        case '#': {
            int p = 0;
            while (q + p < len) {
                if (defbufpos < sizeof(defbuf) - 1) defbuf[defbufpos++] = sonfile[q + p];
                if (sonfile[q + p] == '\n') break;
                p++;
            }
            defbuf[defbufpos] = '\0';
            p = 7;
            while (defbuf[p] == ' ') p++;
            while (p < defbufpos) {
                if (defbuf[p] == ' ') break;
                if (varbufpos < sizeof(varbuf) - 1) varbuf[varbufpos++] = defbuf[p];
                p++;
            }
            p++;
            while (p < defbufpos) {
                if (defbuf[p] == '\n') break;
                if (valbufpos < sizeof(valbuf) - 1) valbuf[valbufpos++] = defbuf[p];
                p++;
            }
            varbuf[varbufpos] = '\0';
            valbuf[valbufpos] = '\0';
            if (strncmp(defbuf, "#define", 7) == 0 && macroc < 1024) {
                macros[macroc].name = xstrdup(varbuf);
                macros[macroc].value = xstrdup(valbuf);
                macroc++;
            }
            varbufpos = 0;
            valbufpos = 0;
            q += p;
            defbufpos = 0;
            break;
        }
        default:
            break;
        }
    }

    for (uint16_t i = 0; i < macroc; i++) {
        char* tmp = replace_all(sonfile, macros[i].name, macros[i].value);
        if (tmp) {
            free(sonfile);
            sonfile = tmp;
            len = strlen(sonfile);
        }
    }

    Token toks[MAXTOK];
    int tokc = 0;
    char tokbuf[512];
    uint16_t tokpos = 0;
    int in_string = 0;
    int escaped = 0;
    int strstart = 0;
    int primstart = 0;

#define FLUSH_PRIM() do { if (tokpos > 0) { tok_push(toks, &tokc, TOK_PRIMITIVE, primstart, q); tokpos = 0; } } while (0)

    for (int q = 0; q < len; ++q) {
        char c = sonfile[q];
        if (in_string) {
            if (escaped) escaped = 0;
            else if (c == '\\') escaped = 1;
            else if (c == '"') { tok_push(toks, &tokc, TOK_STRING, strstart, q); in_string = 0; }
            continue;
        }
        switch (c) {
        case ';':
            FLUSH_PRIM();
            while (q < len && sonfile[q] != '\n') q++;
            break;
        case '#':
            FLUSH_PRIM();
            while (q < len && sonfile[q] != '\n') q++;
            break;
        case '{': FLUSH_PRIM(); tok_push(toks, &tokc, TOK_OBJ_OPEN, q, q + 1); break;
        case '}': FLUSH_PRIM(); tok_push(toks, &tokc, TOK_OBJ_CLOSE, q, q + 1); break;
        case '[': FLUSH_PRIM(); tok_push(toks, &tokc, TOK_ARR_OPEN, q, q + 1); break;
        case ']': FLUSH_PRIM(); tok_push(toks, &tokc, TOK_ARR_CLOSE, q, q + 1); break;
        case '"':
            FLUSH_PRIM();
            strstart = q + 1;
            tokpos = 0;
            in_string = 1;
            break;
        case ':': FLUSH_PRIM(); tok_push(toks, &tokc, TOK_COLON, q, q + 1); break;
        case ',': FLUSH_PRIM(); tok_push(toks, &tokc, TOK_COMMA, q, q + 1); break;
        case '!': {
            FLUSH_PRIM();
            int start = q;
            while (q < len && sonfile[q] != '\n' && sonfile[q] != ':') q++;
            tok_push(toks, &tokc, TOK_EXPR, start, q);
            break;
        }
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            FLUSH_PRIM();
            break;
        default:
            if (tokpos == 0) primstart = q;
            if (tokpos < sizeof(tokbuf) - 1) tokbuf[tokpos++] = c;
            break;
        }
    }
#undef FLUSH_PRIM

    cSON_Item* root = build_tree(toks, tokc, sonfile);
    if (root) print_item(root, 0);

    printf("macros: %u\n", (unsigned)macroc);
    for (uint16_t i = 0; i < macroc; i++) {
        printf("  %s = %s\n", macros[i].name, macros[i].value);
        free(macros[i].name);
        free(macros[i].value);
    }

    cSON_Delete(root);
    free(sonfile);
    return 1;
naxyi:
    fprintf(stderr, "FUCK!\n");
    if (sonfile) free(sonfile);
    return 0;
}
