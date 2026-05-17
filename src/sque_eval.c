#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "sque_eval.h"

#define MAX_LINE 4096

/* ---- helpers ---- */

static int is_space(char c)
{
    return c == ' ' || c == '\t';
}

static int ci_eq(const char *a, const char *b)
{
    return _strnicmp(a, b, strlen(b) + 1) == 0;
}

static int ci_eq_n(const char *a, const char *b, int n)
{
    return _strnicmp(a, b, n) == 0;
}

static int is_known_var(const char *name)
{
    if (ci_eq(name, "systemver")) return 1;
    if (ci_eq(name, "os")) return 1;
    if (ci_eq(name, "version")) return 1;
    if (ci_eq(name, "packagesize")) return 1;
    return 0;
}

static int is_readonly_var(const char *name)
{
    if (ci_eq(name, "systemver")) return 1;
    if (ci_eq(name, "os")) return 1;
    return 0;
}

static const char *get_var_value(const SqueEvalContext *ctx, const char *name)
{
    if (ci_eq(name, "systemver")) return ctx->systemver;
    if (ci_eq(name, "os")) return ctx->os;
    if (ci_eq(name, "version")) return ctx->version;
    if (ci_eq(name, "packagesize")) return ctx->packagesize;
    return NULL;
}

static void set_var_value(SqueEvalContext *ctx, const char *name, const char *value)
{
    if (ci_eq(name, "version")) {
        strncpy(ctx->version, value, sizeof(ctx->version) - 1);
        ctx->version[sizeof(ctx->version) - 1] = '\0';
    } else if (ci_eq(name, "packagesize")) {
        strncpy(ctx->packagesize, value, sizeof(ctx->packagesize) - 1);
        ctx->packagesize[sizeof(ctx->packagesize) - 1] = '\0';
    }
}

static void copy_str(char *dst, const char *src, int size)
{
    strncpy(dst, src, size - 1);
    dst[size - 1] = '\0';
}

/* ---- System Version Detection ---- */

int sque_get_systemver(char *buf, int buf_size)
{
    OSVERSIONINFOEXA osvi;
    const char *ver_str;

    memset(&osvi, 0, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (!GetVersionExA((OSVERSIONINFOA *)&osvi)) {
        if (buf_size > 0) buf[0] = '\0';
        return 0;
    }

    if (osvi.dwMajorVersion == 5) {
        if (osvi.dwMinorVersion == 1) ver_str = "xp";
        else if (osvi.dwMinorVersion == 2) ver_str = "xp64";
        else ver_str = "xp";
    } else if (osvi.dwMajorVersion == 6) {
        if (osvi.dwMinorVersion == 0) ver_str = "vista";
        else if (osvi.dwMinorVersion == 1) ver_str = "7";
        else if (osvi.dwMinorVersion == 2) ver_str = "8";
        else if (osvi.dwMinorVersion == 3) ver_str = "8.1";
        else ver_str = "vista";
    } else if (osvi.dwMajorVersion == 10) {
        if (osvi.dwBuildNumber >= 22000) ver_str = "11";
        else ver_str = "10";
    } else {
        ver_str = "10";
    }

    copy_str(buf, ver_str, buf_size);
    return (int)strlen(buf);
}

/* ---- Conditional Line Parser ---- */

int sque_eval_line(const char *line, SqueEvalContext *ctx)
{
    const char *p;
    char lvar_name[64];
    char rval_str[256];
    char assign_target[64];
    char assign_value[256];
    char else_target[64];
    char else_value[256];
    char set_items[8][64];
    int set_count;
    int has_else;
    int i;
    int op_is_eq;
    int condition_true;
    int is_set_compare;

    if (!line || !ctx) return 0;

    p = line;

    /* State 0: skip leading whitespace, expect "if " */
    while (*p && is_space(*p)) p++;
    if (!(*p)) return 0;
    if (!ci_eq_n(p, "if ", 3)) return 0;
    p += 3;

    /* State 1: expect "{var}" */
    while (*p && is_space(*p)) p++;
    if (*p != '{') return 0;
    p++;
    {
        int i_dst = 0;
        while (*p && *p != '}' && i_dst < (int)sizeof(lvar_name) - 1) {
            lvar_name[i_dst++] = *p++;
        }
        lvar_name[i_dst] = '\0';
    }
    if (*p != '}') return 0;
    p++;

    if (!is_known_var(lvar_name)) return 0;

    /* State 2: expect "==" or "!=" */
    while (*p && is_space(*p)) p++;
    if (ci_eq_n(p, "==", 2)) {
        op_is_eq = 1;
        p += 2;
    } else if (ci_eq_n(p, "!=", 2)) {
        op_is_eq = 0;
        p += 2;
    } else {
        return 0;
    }

    /* State 3: expect '"' or '(' */
    while (*p && is_space(*p)) p++;

    is_set_compare = 0;
    set_count = 0;
    rval_str[0] = '\0';

    if (*p == '(') {
        is_set_compare = 1;
        p++;
        while (*p && *p != ')') {
            while (*p && is_space(*p)) p++;
            if (*p == '"') {
                p++;
                i = 0;
                while (*p && *p != '"' && i < (int)sizeof(set_items[0]) - 1) {
                    set_items[set_count][i++] = *p++;
                }
                set_items[set_count][i] = '\0';
                if (*p == '"') p++;
                if (set_count < 7) set_count++;
                while (*p && (is_space(*p) || *p == ',')) p++;
            } else if (*p == ',') {
                p++;
            } else if (*p == ')') {
                break;
            } else {
                return 0;
            }
        }
        if (*p == ')') p++;
    } else if (*p == '"') {
        p++;
        i = 0;
        while (*p && *p != '"' && i < (int)sizeof(rval_str) - 1) {
            rval_str[i++] = *p++;
        }
        rval_str[i] = '\0';
        if (*p != '"') return 0;
        p++;
    } else {
        return 0;
    }

    /* State 4: evaluate condition */
    {
        const char *lval = get_var_value(ctx, lvar_name);

        condition_true = 0;
        if (is_set_compare) {
            for (i = 0; i < set_count; i++) {
                if (ci_eq(lval, set_items[i])) {
                    condition_true = 1;
                    break;
                }
            }
            if (!op_is_eq) condition_true = !condition_true;
        } else {
            if (ci_eq(lval, rval_str)) {
                condition_true = 1;
            }
            if (!op_is_eq) condition_true = !condition_true;
        }
    }

    /* State 5: expect "then " */
    while (*p && is_space(*p)) p++;
    if (!ci_eq_n(p, "then ", 5)) return 0;
    p += 5;

    /* State 6: then-assignment: {version}|{packagesize} = "..." */
    while (*p && is_space(*p)) p++;
    if (*p != '{') return 0;
    p++;
    {
        int i_dst = 0;
        while (*p && *p != '}' && i_dst < (int)sizeof(assign_target) - 1) {
            assign_target[i_dst++] = *p++;
        }
        assign_target[i_dst] = '\0';
    }
    if (*p != '}') return 0;
    p++;

    if (!is_known_var(assign_target)) return 0;
    if (is_readonly_var(assign_target)) return 0;

    while (*p && is_space(*p)) p++;
    if (*p != '=') return 0;
    p++;
    while (*p && is_space(*p)) p++;
    if (*p != '"') return 0;
    p++;

    i = 0;
    while (*p && *p != '"' && i < (int)sizeof(assign_value) - 1) {
        assign_value[i++] = *p++;
    }
    assign_value[i] = '\0';
    if (*p != '"') return 0;
    p++;

    /* State 7: optional else */
    has_else = 0;
    else_target[0] = '\0';
    else_value[0] = '\0';

    while (*p && is_space(*p)) p++;
    if (*p == ';') {
        /* no else */
    } else if (ci_eq_n(p, "else ", 5)) {
        has_else = 1;
        p += 5;

        while (*p && is_space(*p)) p++;
        if (*p != '{') return 0;
        p++;
        {
            int i_dst = 0;
            while (*p && *p != '}' && i_dst < (int)sizeof(else_target) - 1) {
                else_target[i_dst++] = *p++;
            }
            else_target[i_dst] = '\0';
        }
        if (*p != '}') return 0;
        p++;

        if (!is_known_var(else_target)) return 0;
        if (is_readonly_var(else_target)) return 0;

        while (*p && is_space(*p)) p++;
        if (*p != '=') return 0;
        p++;
        while (*p && is_space(*p)) p++;
        if (*p != '"') return 0;
        p++;

        i = 0;
        while (*p && *p != '"' && i < (int)sizeof(else_value) - 1) {
            else_value[i++] = *p++;
        }
        else_value[i] = '\0';
        if (*p != '"') return 0;
        p++;
    } else {
        return 0;
    }

    /* State 8: semicolon */
    while (*p && is_space(*p)) p++;
    if (*p != ';') return 0;

    /* Apply */
    if (condition_true) {
        set_var_value(ctx, assign_target, assign_value);
    } else if (has_else) {
        set_var_value(ctx, else_target, else_value);
    }

    return 1;
}

/* ---- Variable Interpolation ---- */

int sque_interpolate(const char *input, const SqueEvalContext *ctx,
                     char *output, int out_size)
{
    const char *p;
    int pos;
    int i;

    if (!input || !ctx || !output || out_size <= 0) return -1;

    p = input;
    pos = 0;

    while (*p) {
        if (*p == '{') {
            const char *start;
            const char *value;
            char name[64];
            int name_len;

            start = p + 1;
            name_len = 0;
            while (start[name_len] && start[name_len] != '}'
                   && name_len < (int)sizeof(name) - 1) {
                name[name_len] = start[name_len];
                name_len++;
            }
            name[name_len] = '\0';

            if (start[name_len] == '}') {
                value = get_var_value(ctx, name);
                if (value) {
                    i = 0;
                    while (value[i] && pos < out_size - 1) {
                        output[pos++] = value[i++];
                    }
                } else {
                    i = 0;
                    while (i <= name_len + 1 && pos < out_size - 1) {
                        output[pos++] = p[i];
                        i++;
                    }
                }
                p = start + name_len + 1;
            } else {
                if (pos < out_size - 1) output[pos++] = *p++;
            }
        } else {
            if (pos < out_size - 1) output[pos++] = *p++;
        }
    }

    output[pos] = '\0';
    return pos;
}

/* ---- Script Section Evaluator ---- */

int sque_eval_script(const char *filepath, SqueEvalContext *ctx)
{
    FILE *fp;
    char raw_line[MAX_LINE];
    int in_script;
    int executed;

    if (!filepath || !ctx) return -1;

    fp = fopen(filepath, "r");
    if (!fp) return -1;

    in_script = 0;
    executed = 0;

    while (fgets(raw_line, sizeof(raw_line), fp)) {
        char line[MAX_LINE];
        int llen;

        strncpy(line, raw_line, sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';
        llen = (int)strlen(line);
        while (llen > 0 && (line[llen-1] == '\r' || line[llen-1] == '\n')) {
            line[--llen] = '\0';
        }

        if (line[0] == '[') {
            if (strcmp(line, "[Script]") == 0) {
                in_script = 1;
            } else {
                in_script = 0;
            }
            continue;
        }

        if (in_script) {
            char *p;

            p = line;
            while (*p == ' ' || *p == '\t') p++;

            if ((*p == 'i' || *p == 'I') && (*(p+1) == 'f' || *(p+1) == 'F') &&
                (*(p+2) == ' ' || *(p+2) == '\t')) {
                sque_eval_line(p, ctx);
                executed++;
            }
        }
    }

    fclose(fp);
    return executed;
}