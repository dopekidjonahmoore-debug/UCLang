#pragma once
/* UCLang Compiled Runtime
 * Standalone C header — no external dependencies.
 * Include once in the generated .c file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef _MSC_VER
#define strdup _strdup
#define snprintf _snprintf
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ── Value type ───────────────────────────────────────── */
typedef enum { VAL_NULL, VAL_INT, VAL_FLOAT, VAL_STRING, VAL_BOOL, VAL_LIST } ValType;

typedef struct Value Value;
typedef struct {
    Value* items;
    size_t len;
    size_t cap;
} ValueList;

struct Value {
    ValType type;
    int64_t i;
    double f;
    char* s;
    bool b;
    ValueList list;
};

/* ── Constructors ─────────────────────────────────────── */
static Value val_int(int64_t v) {
    Value r = {VAL_INT, v, 0, NULL, 0, {NULL,0,0}};
    return r;
}
static Value val_float(double v) {
    Value r = {VAL_FLOAT, 0, v, NULL, 0, {NULL,0,0}};
    return r;
}
static Value val_string(const char* v) {
    Value r = {VAL_STRING, 0, 0, v ? strdup(v) : NULL, 0, {NULL,0,0}};
    return r;
}
static Value val_bool(bool v) {
    Value r = {VAL_BOOL, 0, 0, NULL, v, {NULL,0,0}};
    return r;
}
static Value val_null(void) {
    Value r = {VAL_NULL, 0, 0, NULL, 0, {NULL,0,0}};
    return r;
}

/* ── Copy / free ──────────────────────────────────────── */
static Value val_copy(Value v) {
    Value r = v;
    if (v.type == VAL_STRING && v.s) r.s = strdup(v.s);
    if (v.type == VAL_LIST && v.list.items) {
        r.list.items = (Value*)malloc(v.list.cap * sizeof(Value));
        memcpy(r.list.items, v.list.items, v.list.len * sizeof(Value));
        for (size_t i = 0; i < v.list.len; i++)
            r.list.items[i] = val_copy(v.list.items[i]);
        r.list.len = v.list.len;
        r.list.cap = v.list.cap;
    }
    return r;
}
static void val_free(Value v) {
    if (v.type == VAL_STRING && v.s) free(v.s);
    if (v.type == VAL_LIST && v.list.items) {
        for (size_t i = 0; i < v.list.len; i++) val_free(v.list.items[i]);
        free(v.list.items);
    }
}

/* ── List operations ──────────────────────────────────── */
static void list_grow(ValueList* l) {
    if (!l->cap) { l->cap = 4; l->items = (Value*)malloc(4 * sizeof(Value)); }
    else { l->cap *= 2; l->items = (Value*)realloc(l->items, l->cap * sizeof(Value)); }
}
static void list_push_val(ValueList* l, Value v) {
    if (l->len >= l->cap) list_grow(l);
    l->items[l->len++] = v;
}

/* ── Arithmetic conversions ───────────────────────────── */
static int64_t val_to_int(Value v) {
    if (v.type == VAL_INT) return v.i;
    if (v.type == VAL_FLOAT) return (int64_t)v.f;
    return 0;
}
static double val_to_float(Value v) {
    if (v.type == VAL_FLOAT) return v.f;
    if (v.type == VAL_INT) return (double)v.i;
    return 0.0;
}

/* ── Operators ────────────────────────────────────────── */
static Value val_add(Value l, Value r) {
    if (l.type == VAL_STRING || r.type == VAL_STRING) {
        const char* ls = (l.type == VAL_STRING && l.s) ? l.s : "";
        const char* rs = (r.type == VAL_STRING && r.s) ? r.s : "";
        char* buf = (char*)malloc(strlen(ls) + strlen(rs) + 1);
        strcpy(buf, ls); strcat(buf, rs);
        Value res = val_string(buf); free(buf);
        return res;
    }
    if (l.type == VAL_INT && r.type == VAL_INT) return val_int(l.i + r.i);
    return val_float(val_to_float(l) + val_to_float(r));
}
static Value val_sub(Value l, Value r) {
    if (l.type == VAL_INT && r.type == VAL_INT) return val_int(l.i - r.i);
    return val_float(val_to_float(l) - val_to_float(r));
}
static Value val_mul(Value l, Value r) {
    if (l.type == VAL_INT && r.type == VAL_INT) return val_int(l.i * r.i);
    return val_float(val_to_float(l) * val_to_float(r));
}
static Value val_div(Value l, Value r) {
    if (l.type == VAL_INT && r.type == VAL_INT) return val_int(l.i / r.i);
    return val_float(val_to_float(l) / val_to_float(r));
}

static bool val_truthy(Value v) {
    if (v.type == VAL_BOOL) return v.b;
    if (v.type == VAL_INT) return v.i != 0;
    if (v.type == VAL_FLOAT) return v.f != 0.0;
    if (v.type == VAL_STRING) return v.s && v.s[0] != '\0';
    return false;
}

static Value val_eq(Value l, Value r) {
    if (l.type != r.type) return val_bool(0);
    switch (l.type) {
        case VAL_INT:    return val_bool(l.i == r.i);
        case VAL_FLOAT:  return val_bool(l.f == r.f);
        case VAL_STRING: return val_bool(l.s && r.s && strcmp(l.s, r.s) == 0);
        case VAL_BOOL:   return val_bool(l.b == r.b);
        case VAL_NULL:   return val_bool(1);
        default:         return val_bool(0);
    }
}
static Value val_neq(Value l, Value r) { Value e = val_eq(l, r); return val_bool(!e.b); }
static Value val_gt(Value l, Value r) {
    if (l.type == VAL_INT && r.type == VAL_INT) return val_bool(l.i > r.i);
    return val_bool(val_to_float(l) > val_to_float(r));
}
static Value val_lt(Value l, Value r) {
    if (l.type == VAL_INT && r.type == VAL_INT) return val_bool(l.i < r.i);
    return val_bool(val_to_float(l) < val_to_float(r));
}
static Value val_gte(Value l, Value r) {
    if (l.type == VAL_INT && r.type == VAL_INT) return val_bool(l.i >= r.i);
    return val_bool(val_to_float(l) >= val_to_float(r));
}
static Value val_lte(Value l, Value r) {
    if (l.type == VAL_INT && r.type == VAL_INT) return val_bool(l.i <= r.i);
    return val_bool(val_to_float(l) <= val_to_float(r));
}

/* ── Boolean operators ────────────────────────────────── */
static Value val_or(Value l, Value r) {
    return val_bool(val_truthy(l) || val_truthy(r));
}
static Value val_and(Value l, Value r) {
    return val_bool(val_truthy(l) && val_truthy(r));
}

/* ── Environment ──────────────────────────────────────── */
typedef struct Env Env;
struct Env {
    Env* parent;
    char** names;
    Value* values;
    size_t count;
    size_t cap;
};

static void env_set(Env* env, const char* name, Value v) {
    for (Env* e = env; e; e = e->parent) {
        for (size_t i = 0; i < e->count; i++) {
            if (strcmp(e->names[i], name) == 0) {
                val_free(e->values[i]);
                e->values[i] = val_copy(v);
                return;
            }
        }
        if (!e->parent) break;
    }
    if (env->count >= env->cap) {
        if (!env->cap) env->cap = 16;
        else env->cap *= 2;
        env->names = (char**)realloc(env->names, env->cap * sizeof(char*));
        env->values = (Value*)realloc(env->values, env->cap * sizeof(Value));
    }
    env->names[env->count] = strdup(name);
    env->values[env->count] = val_copy(v);
    env->count++;
}
static Value env_get(Env* env, const char* name) {
    for (Env* e = env; e; e = e->parent) {
        for (size_t i = 0; i < e->count; i++)
            if (strcmp(e->names[i], name) == 0) return val_copy(e->values[i]);
        if (!e->parent) break;
    }
    return val_null();
}

/* ── Print / Input ────────────────────────────────────── */
static void v2s(Value v, char* buf, size_t sz) {
    switch (v.type) {
        case VAL_NULL:   snprintf(buf, sz, "null"); break;
        case VAL_INT:    snprintf(buf, sz, "%lld", (long long)v.i); break;
        case VAL_FLOAT:  snprintf(buf, sz, "%g", v.f); break;
        case VAL_STRING: snprintf(buf, sz, "%s", v.s ? v.s : ""); break;
        case VAL_BOOL:   snprintf(buf, sz, "%s", v.b ? "true" : "false"); break;
        case VAL_LIST: {
            size_t pos = 0;
            pos += snprintf(buf + pos, sz - pos, "[");
            for (size_t i = 0; i < v.list.len && pos < sz; i++) {
                if (i > 0) pos += snprintf(buf + pos, sz - pos, ", ");
                char tmp[256]; v2s(v.list.items[i], tmp, sizeof(tmp));
                pos += snprintf(buf + pos, sz - pos, "%s", tmp);
            }
            snprintf(buf + pos, sz - pos, "]");
            break;
        }
    }
}
static void ucl_print(Value v) {
    char buf[4096]; v2s(v, buf, sizeof(buf)); fputs(buf, stdout); fflush(stdout);
}
static Value ucl_input(Value prompt) {
    char pbuf[4096]; v2s(prompt, pbuf, sizeof(pbuf));
    fputs(pbuf, stdout); fflush(stdout);
    char buf[1024];
    if (!fgets(buf, sizeof(buf), stdin)) return val_string("");
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
    return val_string(buf);
}

/* ── Builtins (called by generated code) ──────────────── */
static Value builtin_list_push(int nargs, Value* args) {
    if (nargs < 1) return val_null();
    ValueList l = { NULL, 0, 0 };
    for (size_t i = 0; i < args[0].list.len; i++)
        list_push_val(&l, val_copy(args[0].list.items[i]));
    for (int i = 1; i < nargs; i++)
        list_push_val(&l, val_copy(args[i]));
    Value r = {VAL_LIST, 0, 0, NULL, 0, l};
    return r;
}
static Value builtin_list_pop(int nargs, Value* args) {
    (void)nargs;
    if (nargs < 1 || args[0].list.len == 0) return val_null();
    ValueList l = { NULL, 0, 0 };
    for (size_t i = 0; i < args[0].list.len - 1; i++)
        list_push_val(&l, val_copy(args[0].list.items[i]));
    Value popped = val_copy(args[0].list.items[args[0].list.len - 1]);
    /* Assign popped to args[1] if provided */
    return popped;
}
static Value builtin_list_len(int nargs, Value* args) {
    (void)nargs;
    if (nargs < 1) return val_int(0);
    return val_int((int64_t)args[0].list.len);
}
static Value builtin_file_read(int nargs, Value* args) {
    (void)nargs;
    if (nargs < 1 || args[0].type != VAL_STRING) return val_string("");
    FILE* f = fopen(args[0].s, "rb");
    if (!f) return val_string("");
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc((size_t)sz + 1);
    fread(buf, 1, (size_t)sz, f); fclose(f);
    buf[sz] = '\0';
    Value r = val_string(buf); free(buf);
    return r;
}
static Value builtin_file_write(int nargs, Value* args) {
    (void)nargs;
    if (nargs < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return val_null();
    FILE* f = fopen(args[0].s, "wb");
    if (!f) return val_null();
    fwrite(args[1].s, 1, strlen(args[1].s), f);
    fclose(f);
    return val_null();
}
static Value builtin_list_get(int nargs, Value* args) {
    (void)nargs;
    if (nargs < 2 || args[0].type != VAL_LIST || args[1].type != VAL_INT) return val_null();
    int64_t idx = args[1].i;
    if (idx < 0 || (size_t)idx >= args[0].list.len) return val_null();
    return val_copy(args[0].list.items[(size_t)idx]);
}

static Value builtin_str_len(int nargs, Value* args) {
    (void)nargs;
    if (nargs < 1 || args[0].type != VAL_STRING) return val_int(0);
    return val_int((int64_t)strlen(args[0].s));
}
static Value builtin_str_get(int nargs, Value* args) {
    (void)nargs;
    if (nargs < 2 || args[0].type != VAL_STRING || args[1].type != VAL_INT) return val_string("");
    int64_t i = args[1].i;
    size_t len = strlen(args[0].s);
    if (i < 0 || (size_t)i >= len) return val_string("");
    char buf[2] = {args[0].s[i], '\0'};
    return val_string(buf);
}
static Value builtin_str_slice(int nargs, Value* args) {
    (void)nargs;
    if (nargs < 3 || args[0].type != VAL_STRING || args[1].type != VAL_INT || args[2].type != VAL_INT)
        return val_string("");
    int64_t start = args[1].i, len = args[2].i;
    size_t slen = strlen(args[0].s);
    if (start < 0) start = 0;
    if ((size_t)start > slen) return val_string("");
    if ((size_t)(start + len) > slen) len = (int64_t)(slen - (size_t)start);
    if (len <= 0) return val_string("");
    char* buf = (char*)malloc((size_t)len + 1);
    memcpy(buf, args[0].s + start, (size_t)len);
    buf[len] = '\0';
    Value r = val_string(buf); free(buf);
    return r;
}
static Value builtin_int_to_str(int nargs, Value* args) {
    (void)nargs;
    if (nargs < 1) return val_string("0");
    if (args[0].type == VAL_INT) {
        char buf[64]; snprintf(buf, sizeof(buf), "%lld", (long long)args[0].i);
        return val_string(buf);
    }
    if (args[0].type == VAL_FLOAT) {
        char buf[64]; snprintf(buf, sizeof(buf), "%g", args[0].f);
        return val_string(buf);
    }
    return val_string("0");
}
static Value builtin_str_to_int(int nargs, Value* args) {
    (void)nargs;
    if (nargs < 1 || args[0].type != VAL_STRING) return val_int(0);
    long long v = strtoll(args[0].s, NULL, 10);
    return val_int((int64_t)v);
}

/* ── Native char-class functions (replace UCLang is_alpha/is_digit/is_alnum) ── */
static Value func_is_digit(struct Env* env, Value c);
static Value func_is_alpha(struct Env* env, Value c);
static Value func_is_alnum(struct Env* env, Value p0) {
    env_set(env, "c", p0);
    return val_bool(val_truthy(func_is_digit(env, p0)) || val_truthy(func_is_alpha(env, p0)));
}
static Value func_is_digit(struct Env* env, Value c) {
    env_set(env, "c", c);
    int64_t ch = 0;
    if (c.type == VAL_STRING && c.s) ch = (unsigned char)c.s[0];
    else if (c.type == VAL_INT) ch = c.i;
    return val_bool(ch >= '0' && ch <= '9');
}
static Value func_is_alpha(struct Env* env, Value c) {
    env_set(env, "c", c);
    int64_t ch = 0;
    if (c.type == VAL_STRING && c.s) ch = (unsigned char)c.s[0];
    else if (c.type == VAL_INT) ch = c.i;
    return val_bool((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_');
}

#ifdef __cplusplus
}
#endif
