/* registry.cpp -- the Windows registry, kept in a TOML file.
 *
 * The registry is real here: what a program writes it reads back, across
 * runs. It lives in `registry.toml` in the current directory (or the file
 * named by $W32L_REGISTRY), read with tomlc99 at first use and rewritten
 * after every change. Keys are TOML tables, nested the way the key path is;
 * the four roots are the top-level tables:
 *
 *     [HKEY_LOCAL_MACHINE.SOFTWARE."Pyro Studios".Commandos]
 *     InstallPath = "C:\\GOG Games\\Commandos"       # REG_SZ
 *     Volume = 7                                       # REG_DWORD
 *     Blob = ["REG_BINARY", "0a1b2c"]                  # anything else:
 *     Paths = ["REG_MULTI_SZ", ["a", "b"]]             #   [type, data]
 *
 * REG_SZ is a TOML string and REG_DWORD a TOML integer; every other type is a
 * two-element array of the type name and its data (hex for binary, a string
 * for REG_EXPAND_SZ, an array of strings for REG_MULTI_SZ, a decimal string
 * for REG_QWORD). The default value of a key is the value named "".
 *
 * Key and value names compare case-insensitively and keep the case they were
 * created with, as on Windows.
 */
#include "platform.h"

#include "../third_party/tomlc99/toml.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

namespace {

struct Node;

struct Value {
    char    *name;
    DWORD    type;
    uint8_t *data;
    DWORD    size;
};

struct Node {
    uint32_t magic;
    char    *name;
    Node    *parent;
    Node   **kids;
    uint32_t nkids, capKids;
    Value   *vals;
    uint32_t nvals, capVals;
};

const uint32_t kMagic = 0x52454731u;           /* 'REG1' */

pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
Node           *g_roots[4];                    /* CLASSES_ROOT, CURRENT_USER, LOCAL_MACHINE, USERS */
int             g_loaded;
const char *const kRootNames[4] = { "HKEY_CLASSES_ROOT", "HKEY_CURRENT_USER",
                                    "HKEY_LOCAL_MACHINE", "HKEY_USERS" };

Node *node_new(const char *name, Node *parent)
{
    Node *n = (Node *)calloc(1, sizeof *n);
    n->magic = kMagic;
    n->name = strdup(name);
    n->parent = parent;
    return n;
}

Node *find_kid(Node *n, const char *name, size_t len)
{
    uint32_t i;
    for (i = 0; i < n->nkids; i++)
        if (strlen(n->kids[i]->name) == len && !strncasecmp(n->kids[i]->name, name, len))
            return n->kids[i];
    return NULL;
}

Node *add_kid(Node *n, const char *name, size_t len)
{
    char *copy = strndup(name, len);
    Node *k;

    if (n->nkids == n->capKids) {
        n->capKids = n->capKids ? n->capKids * 2 : 4;
        n->kids = (Node **)realloc(n->kids, n->capKids * sizeof(Node *));
    }
    k = node_new(copy, n);
    free(copy);
    n->kids[n->nkids++] = k;
    return k;
}

Value *find_val(Node *n, const char *name)
{
    uint32_t i;
    for (i = 0; i < n->nvals; i++)
        if (!strcasecmp(n->vals[i].name, name))
            return &n->vals[i];
    return NULL;
}

Value *put_val(Node *n, const char *name, DWORD type, const uint8_t *data, DWORD size)
{
    Value *v = find_val(n, name);

    if (!v) {
        if (n->nvals == n->capVals) {
            n->capVals = n->capVals ? n->capVals * 2 : 4;
            n->vals = (Value *)realloc(n->vals, n->capVals * sizeof(Value));
        }
        v = &n->vals[n->nvals++];
        v->name = strdup(name);
        v->data = NULL;
    }
    free(v->data);
    v->type = type;
    v->size = size;
    v->data = (uint8_t *)malloc(size ? size : 1);
    if (size)
        memcpy(v->data, data, size);
    return v;
}

/* ---- the file --------------------------------------------------------------------- */

const char *file_name(void)
{
    const char *env = getenv("W32L_REGISTRY");
    return env && *env ? env : "registry.toml";
}

int hexval(int c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

uint8_t *unhex(const char *s, DWORD *size)
{
    size_t n = strlen(s) / 2, i;
    uint8_t *b = (uint8_t *)malloc(n ? n : 1);
    for (i = 0; i < n; i++) {
        int hi = hexval(s[2 * i]), lo = hexval(s[2 * i + 1]);
        b[i] = (uint8_t)(((hi < 0 ? 0 : hi) << 4) | (lo < 0 ? 0 : lo));
    }
    *size = (DWORD)n;
    return b;
}

DWORD type_of(const char *name)
{
    static const struct { const char *n; DWORD t; } t[] = {
        { "REG_NONE", REG_NONE }, { "REG_SZ", REG_SZ }, { "REG_EXPAND_SZ", REG_EXPAND_SZ },
        { "REG_BINARY", REG_BINARY }, { "REG_DWORD", REG_DWORD }, { "REG_MULTI_SZ", REG_MULTI_SZ },
        { "REG_QWORD", REG_QWORD },
    };
    size_t i;
    for (i = 0; i < sizeof t / sizeof t[0]; i++)
        if (!strcmp(t[i].n, name))
            return t[i].t;
    if (!strncmp(name, "REG_", 4))
        return (DWORD)strtoul(name + 4, NULL, 10);
    return REG_BINARY;
}

const char *name_of(DWORD type, char *buf)
{
    switch (type) {
    case REG_NONE:      return "REG_NONE";
    case REG_SZ:        return "REG_SZ";
    case REG_EXPAND_SZ: return "REG_EXPAND_SZ";
    case REG_BINARY:    return "REG_BINARY";
    case REG_DWORD:     return "REG_DWORD";
    case REG_MULTI_SZ:  return "REG_MULTI_SZ";
    case REG_QWORD:     return "REG_QWORD";
    default:            snprintf(buf, 24, "REG_%u", (unsigned)type); return buf;
    }
}

void load_value(Node *n, const char *name, toml_table_t *tab)
{
    toml_datum_t d;
    toml_array_t *arr;

    d = toml_string_in(tab, name);
    if (d.ok) {
        put_val(n, name, REG_SZ, (const uint8_t *)d.u.s, (DWORD)strlen(d.u.s) + 1);
        free(d.u.s);
        return;
    }
    d = toml_int_in(tab, name);
    if (d.ok) {
        uint32_t v = (uint32_t)d.u.i;
        put_val(n, name, REG_DWORD, (const uint8_t *)&v, 4);
        return;
    }
    arr = toml_array_in(tab, name);
    if (arr && toml_array_nelem(arr) == 2) {
        toml_datum_t t = toml_string_at(arr, 0);
        DWORD type;
        if (!t.ok)
            return;
        type = type_of(t.u.s);
        free(t.u.s);
        if (type == REG_MULTI_SZ) {
            toml_array_t *list = toml_array_at(arr, 1);
            uint8_t *buf = NULL;
            size_t   len = 0;
            int      i;
            for (i = 0; list && i < toml_array_nelem(list); i++) {
                toml_datum_t e = toml_string_at(list, i);
                if (!e.ok)
                    continue;
                buf = (uint8_t *)realloc(buf, len + strlen(e.u.s) + 1);
                memcpy(buf + len, e.u.s, strlen(e.u.s) + 1);
                len += strlen(e.u.s) + 1;
                free(e.u.s);
            }
            buf = (uint8_t *)realloc(buf, len + 1);
            buf[len++] = 0;                 /* the list's own terminator */
            put_val(n, name, type, buf, (DWORD)len);
            free(buf);
        } else {
            toml_datum_t e = toml_string_at(arr, 1);
            if (!e.ok)
                return;
            if (type == REG_SZ || type == REG_EXPAND_SZ) {
                put_val(n, name, type, (const uint8_t *)e.u.s, (DWORD)strlen(e.u.s) + 1);
            } else if (type == REG_QWORD) {
                uint64_t q = strtoull(e.u.s, NULL, 10);
                put_val(n, name, type, (const uint8_t *)&q, 8);
            } else if (type == REG_DWORD) {
                uint32_t v = (uint32_t)strtoul(e.u.s, NULL, 10);
                put_val(n, name, type, (const uint8_t *)&v, 4);
            } else {
                DWORD size;
                uint8_t *b = unhex(e.u.s, &size);
                put_val(n, name, type, b, size);
                free(b);
            }
            free(e.u.s);
        }
    }
}

void load_table(Node *n, toml_table_t *tab)
{
    int i;
    const char *key;

    for (i = 0; (key = toml_key_in(tab, i)) != NULL; i++) {
        toml_table_t *sub = toml_table_in(tab, key);
        if (sub) {
            Node *k = find_kid(n, key, strlen(key));
            if (!k)
                k = add_kid(n, key, strlen(key));
            load_table(k, sub);
        } else {
            load_value(n, key, tab);
        }
    }
}

void load(void)
{
    char          err[200];
    FILE         *fp;
    toml_table_t *top;
    int           i;

    g_loaded = 1;
    for (i = 0; i < 4; i++)
        g_roots[i] = node_new(kRootNames[i], NULL);
    fp = fopen(file_name(), "r");
    if (!fp)
        return;
    top = toml_parse_file(fp, err, sizeof err);
    fclose(fp);
    if (!top) {
        w32l_plat_log("registry: %s: %s (starting empty)", file_name(), err);
        return;
    }
    for (i = 0; i < 4; i++) {
        toml_table_t *t = toml_table_in(top, kRootNames[i]);
        if (t)
            load_table(g_roots[i], t);
    }
    toml_free(top);
}

void emit_name(FILE *f, const char *s, int quoted = 0)
{
    const char *p;
    int bare = *s != 0 && !quoted;

    for (p = s; *p; p++)
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_' || *p == '-'))
            bare = 0;
    if (bare) {
        fputs(s, f);
        return;
    }
    fputc('"', f);
    for (p = s; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '"' || c == '\\')
            fprintf(f, "\\%c", c);
        else if (c == '\n')
            fputs("\\n", f);
        else if (c == '\t')
            fputs("\\t", f);
        else if (c < 0x20 || c == 0x7F)
            fprintf(f, "\\u%04X", c);
        else
            fputc(c, f);
    }
    fputc('"', f);
}

void emit_path(FILE *f, Node *n)
{
    if (n->parent) {
        emit_path(f, n->parent);
        fputc('.', f);
    }
    emit_name(f, n->name);
}

void emit_value(FILE *f, const Value *v)
{
    char tn[24];

    emit_name(f, v->name);
    fputs(" = ", f);
    if (v->type == REG_SZ) {
        char *s = strndup((const char *)v->data, v->size);
        emit_name(f, s, 1);
        free(s);
    } else if (v->type == REG_DWORD && v->size == 4) {
        uint32_t d;
        memcpy(&d, v->data, 4);
        fprintf(f, "%u", (unsigned)d);
    } else if (v->type == REG_EXPAND_SZ) {
        char *s = strndup((const char *)v->data, v->size);
        fprintf(f, "[\"REG_EXPAND_SZ\", ");
        emit_name(f, s, 1);
        fputs("]", f);
        free(s);
    } else if (v->type == REG_MULTI_SZ) {
        DWORD i = 0;
        fputs("[\"REG_MULTI_SZ\", [", f);
        while (i < v->size && v->data[i]) {
            const char *s = (const char *)v->data + i;
            size_t l = strnlen(s, v->size - i);
            if (i)
                fputs(", ", f);
            emit_name(f, s, 1);
            i += (DWORD)l + 1;
        }
        fputs("]]", f);
    } else if (v->type == REG_QWORD && v->size == 8) {
        uint64_t q;
        memcpy(&q, v->data, 8);
        fprintf(f, "[\"REG_QWORD\", \"%llu\"]", (unsigned long long)q);
    } else {
        DWORD i;
        fprintf(f, "[\"%s\", \"", name_of(v->type, tn));
        for (i = 0; i < v->size; i++)
            fprintf(f, "%02x", v->data[i]);
        fputs("\"]", f);
    }
    fputc('\n', f);
}

void emit_node(FILE *f, Node *n)
{
    uint32_t i;

    if (n->parent && (n->nvals || !n->nkids)) {
        fputs("\n[", f);
        emit_path(f, n);
        fputs("]\n", f);
        for (i = 0; i < n->nvals; i++)
            emit_value(f, &n->vals[i]);
    }
    for (i = 0; i < n->nkids; i++)
        emit_node(f, n->kids[i]);
}

/* Written beside the file and renamed over it, so a crash leaves the old one. */
void save(void)
{
    char   tmp[1100];
    FILE  *f;
    int    i;

    snprintf(tmp, sizeof tmp, "%s.tmp", file_name());
    f = fopen(tmp, "w");
    if (!f)
        return;
    fputs("# The registry of the program running on win32_layer. Edit while it is not running.\n", f);
    for (i = 0; i < 4; i++)
        emit_node(f, g_roots[i]);
    if (fclose(f) == 0)
        rename(tmp, file_name());
}

/* ---- handles ------------------------------------------------------------------------ */

Node *resolve(HKEY key)
{
    uintptr_t k = (uintptr_t)key;

    if (!g_loaded)
        load();
    if (k >= 0x80000000u && k <= 0x80000003u)
        return g_roots[k - 0x80000000u];
    if (k == 0x80000005u)
        return g_roots[2];                      /* CURRENT_CONFIG: under LOCAL_MACHINE */
    if (k < 0x10000)
        return NULL;
    return ((Node *)key)->magic == kMagic ? (Node *)key : NULL;
}

/* Follow `sub` below `n` (components split at '\'); optionally creating. */
Node *walk(Node *n, const char *sub, int create, int *created)
{
    const char *p = sub ? sub : "";

    if (created)
        *created = 0;
    while (*p) {
        const char *end;
        Node       *k;
        while (*p == '\\')
            p++;
        if (!*p)
            break;
        end = strchr(p, '\\');
        if (!end)
            end = p + strlen(p);
        k = find_kid(n, p, (size_t)(end - p));
        if (!k) {
            if (!create)
                return NULL;
            k = add_kid(n, p, (size_t)(end - p));
            if (created)
                *created = 1;
        }
        n = k;
        p = end;
    }
    return n;
}

LONG create_or_open(HKEY key, LPCSTR sub, PHKEY out, int create, LPDWORD disposition)
{
    Node *n, *k;
    int   created = 0;

    pthread_mutex_lock(&g_lock);
    n = resolve(key);
    if (!n) {
        pthread_mutex_unlock(&g_lock);
        return ERROR_INVALID_HANDLE;
    }
    k = walk(n, sub, create, &created);
    if (k && created)
        save();
    pthread_mutex_unlock(&g_lock);
    if (!k)
        return ERROR_FILE_NOT_FOUND;
    if (out)
        *out = (HKEY)k;
    if (disposition)
        *disposition = created ? REG_CREATED_NEW_KEY : REG_OPENED_EXISTING_KEY;
    return ERROR_SUCCESS;
}

} // namespace

LONG WINAPI RegCreateKeyExA(HKEY key, LPCSTR sub, DWORD reserved, LPSTR cls, DWORD options,
                            DWORD access, LPSECURITY_ATTRIBUTES attr, PHKEY out, LPDWORD disposition)
{
    (void)reserved; (void)cls; (void)options; (void)access; (void)attr;
    return create_or_open(key, sub, out, 1, disposition);
}

LONG WINAPI RegCreateKeyA(HKEY key, LPCSTR sub, PHKEY out)
{
    return create_or_open(key, sub, out, 1, NULL);
}

LONG WINAPI RegOpenKeyExA(HKEY key, LPCSTR sub, DWORD options, DWORD access, PHKEY out)
{
    (void)options; (void)access;
    return create_or_open(key, sub, out, 0, NULL);
}

LONG WINAPI RegOpenKeyA(HKEY key, LPCSTR sub, PHKEY out)
{
    return create_or_open(key, sub, out, 0, NULL);
}

/* Handles are the nodes themselves and are never freed, so closing is
 * only a check. */
LONG WINAPI RegCloseKey(HKEY key)
{
    Node *n;

    pthread_mutex_lock(&g_lock);
    n = resolve(key);
    pthread_mutex_unlock(&g_lock);
    return n ? ERROR_SUCCESS : ERROR_INVALID_HANDLE;
}

LONG WINAPI RegFlushKey(HKEY key)
{
    (void)key;
    return ERROR_SUCCESS;
}

LONG WINAPI RegQueryValueExA(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type,
                             LPBYTE data, LPDWORD dataLen)
{
    Node  *n;
    Value *v;
    LONG   r = ERROR_SUCCESS;

    (void)reserved;
    pthread_mutex_lock(&g_lock);
    n = resolve(key);
    v = n ? find_val(n, name ? name : "") : NULL;
    if (!n) {
        r = ERROR_INVALID_HANDLE;
    } else if (!v) {
        r = ERROR_FILE_NOT_FOUND;
    } else {
        if (type)
            *type = v->type;
        if (dataLen) {
            if (data && *dataLen < v->size)
                r = ERROR_MORE_DATA;
            else if (data)
                memcpy(data, v->data, v->size);
            *dataLen = v->size;
        }
    }
    pthread_mutex_unlock(&g_lock);
    return r;
}

LONG WINAPI RegSetValueExA(HKEY key, LPCSTR name, DWORD reserved, DWORD type,
                           const BYTE *data, DWORD dataLen)
{
    Node *n;

    (void)reserved;
    pthread_mutex_lock(&g_lock);
    n = resolve(key);
    if (!n) {
        pthread_mutex_unlock(&g_lock);
        return ERROR_INVALID_HANDLE;
    }
    if ((type == REG_SZ || type == REG_EXPAND_SZ) && dataLen && data[dataLen - 1] != 0) {
        /* A string stored without its terminator is read back with one. */
        uint8_t *t = (uint8_t *)malloc(dataLen + 1);
        memcpy(t, data, dataLen);
        t[dataLen] = 0;
        put_val(n, name ? name : "", type, t, dataLen + 1);
        free(t);
    } else {
        put_val(n, name ? name : "", type, data, dataLen);
    }
    save();
    pthread_mutex_unlock(&g_lock);
    return ERROR_SUCCESS;
}

LONG WINAPI RegDeleteValueA(HKEY key, LPCSTR name)
{
    Node    *n;
    uint32_t i;
    LONG     r = ERROR_FILE_NOT_FOUND;

    pthread_mutex_lock(&g_lock);
    n = resolve(key);
    if (!n)
        r = ERROR_INVALID_HANDLE;
    for (i = 0; n && i < n->nvals; i++)
        if (!strcasecmp(n->vals[i].name, name ? name : "")) {
            free(n->vals[i].name);
            free(n->vals[i].data);
            memmove(&n->vals[i], &n->vals[i + 1], (n->nvals - i - 1) * sizeof(Value));
            n->nvals--;
            save();
            r = ERROR_SUCCESS;
            break;
        }
    pthread_mutex_unlock(&g_lock);
    return r;
}

/* A key and everything below it (as on Windows 9x). The nodes are not freed:
 * a handle the program still holds must not dangle. */
LONG WINAPI RegDeleteKeyA(HKEY key, LPCSTR sub)
{
    Node    *n, *k;
    uint32_t i;
    LONG     r = ERROR_FILE_NOT_FOUND;

    pthread_mutex_lock(&g_lock);
    n = resolve(key);
    k = n ? walk(n, sub, 0, NULL) : NULL;
    if (!n)
        r = ERROR_INVALID_HANDLE;
    else if (k && k->parent) {
        Node *p = k->parent;
        for (i = 0; i < p->nkids; i++)
            if (p->kids[i] == k) {
                memmove(&p->kids[i], &p->kids[i + 1], (p->nkids - i - 1) * sizeof(Node *));
                p->nkids--;
                break;
            }
        save();
        r = ERROR_SUCCESS;
    }
    pthread_mutex_unlock(&g_lock);
    return r;
}

LONG WINAPI RegEnumValueA(HKEY key, DWORD index, LPSTR name, LPDWORD nameLen, LPDWORD reserved,
                          LPDWORD type, LPBYTE data, LPDWORD dataLen)
{
    Node  *n;
    Value *v;
    LONG   r = ERROR_SUCCESS;

    (void)reserved;
    pthread_mutex_lock(&g_lock);
    n = resolve(key);
    if (!n) {
        r = ERROR_INVALID_HANDLE;
    } else if (index >= n->nvals) {
        r = ERROR_NO_MORE_ITEMS;
    } else {
        v = &n->vals[index];
        if (!nameLen || !name || *nameLen < strlen(v->name) + 1) {
            r = ERROR_MORE_DATA;
        } else if (data && dataLen && *dataLen < v->size) {
            if (dataLen)
                *dataLen = v->size;
            r = ERROR_MORE_DATA;
        } else {
            strcpy(name, v->name);
            *nameLen = (DWORD)strlen(v->name);
            if (type)
                *type = v->type;
            if (data && dataLen)
                memcpy(data, v->data, v->size);
            if (dataLen)
                *dataLen = v->size;
        }
    }
    pthread_mutex_unlock(&g_lock);
    return r;
}

LONG WINAPI RegEnumKeyExA(HKEY key, DWORD index, LPSTR name, LPDWORD nameLen, LPDWORD reserved,
                          LPSTR cls, LPDWORD clsLen, LPFILETIME lastWrite)
{
    Node *n;
    LONG  r = ERROR_SUCCESS;

    (void)reserved;
    pthread_mutex_lock(&g_lock);
    n = resolve(key);
    if (!n) {
        r = ERROR_INVALID_HANDLE;
    } else if (index >= n->nkids) {
        r = ERROR_NO_MORE_ITEMS;
    } else {
        const char *kn = n->kids[index]->name;
        if (!name || !nameLen || *nameLen < strlen(kn) + 1) {
            r = ERROR_MORE_DATA;
        } else {
            strcpy(name, kn);
            *nameLen = (DWORD)strlen(kn);
            if (cls && clsLen && *clsLen)
                cls[0] = 0;
            if (clsLen)
                *clsLen = 0;
            if (lastWrite)
                memset(lastWrite, 0, sizeof *lastWrite);
        }
    }
    pthread_mutex_unlock(&g_lock);
    return r;
}

LONG WINAPI RegQueryInfoKeyA(HKEY key, LPSTR cls, LPDWORD clsLen, LPDWORD reserved,
                             LPDWORD subKeys, LPDWORD maxSubKey, LPDWORD maxClass,
                             LPDWORD values, LPDWORD maxValueName, LPDWORD maxValueData,
                             LPDWORD secDesc, LPFILETIME lastWrite)
{
    Node    *n;
    uint32_t i;
    DWORD    longestKey = 0, longestName = 0, longestData = 0;

    (void)reserved;
    pthread_mutex_lock(&g_lock);
    n = resolve(key);
    if (!n) {
        pthread_mutex_unlock(&g_lock);
        return ERROR_INVALID_HANDLE;
    }
    for (i = 0; i < n->nkids; i++)
        if (strlen(n->kids[i]->name) > longestKey)
            longestKey = (DWORD)strlen(n->kids[i]->name);
    for (i = 0; i < n->nvals; i++) {
        if (strlen(n->vals[i].name) > longestName)
            longestName = (DWORD)strlen(n->vals[i].name);
        if (n->vals[i].size > longestData)
            longestData = n->vals[i].size;
    }
    if (cls && clsLen && *clsLen)
        cls[0] = 0;
    if (clsLen)
        *clsLen = 0;
    if (subKeys)       *subKeys = n->nkids;
    if (maxSubKey)     *maxSubKey = longestKey;
    if (maxClass)      *maxClass = 0;
    if (values)        *values = n->nvals;
    if (maxValueName)  *maxValueName = longestName;
    if (maxValueData)  *maxValueData = longestData;
    if (secDesc)       *secDesc = 0;
    if (lastWrite)     memset(lastWrite, 0, sizeof *lastWrite);
    pthread_mutex_unlock(&g_lock);
    return ERROR_SUCCESS;
}
