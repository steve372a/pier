#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>

#define MAX_PATH_LEN 1024
#define MAX_LINE 4096
#define MAX_ERR_STR 512
#define MAX_CMD_LEN 8192

static int g_line_count;
static int g_error_count;
static char g_pier_root[MAX_PATH_LEN];

static char err_missing_semicolon[MAX_ERR_STR];
static char err_single_equal[MAX_ERR_STR];
static char err_quote_mismatch[MAX_ERR_STR];
static char err_paren_mismatch[MAX_ERR_STR];
static char err_undefined_var[MAX_ERR_STR];
static char err_missing_then[MAX_ERR_STR];
static char err_readonly_assign[MAX_ERR_STR];
static char err_not_in_script[MAX_ERR_STR];
static char msg_no_errors[MAX_ERR_STR];

/* ---- Pier root detection from exe path ---- */

static void detect_pier_root(void)
{
    char exe_path[MAX_PATH_LEN];
    char *last_slash;

    GetModuleFileName(NULL, exe_path, sizeof(exe_path));
    last_slash = strrchr(exe_path, '\\');
    if (last_slash) {
        *last_slash = '\0';
        /* Go up from bin\ to root */
        last_slash = strrchr(exe_path, '\\');
        if (last_slash) {
            *last_slash = '\0';
        }
    }
    strncpy(g_pier_root, exe_path, sizeof(g_pier_root) - 1);
    g_pier_root[sizeof(g_pier_root) - 1] = '\0';
}

/* ---- vecho output ---- */

static char *utf8_to_acp(const char *utf8);

static void vecho_line(const char *message)
{
    char exe_path[MAX_PATH_LEN];
    char cmdline[MAX_CMD_LEN];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    snprintf(exe_path, sizeof(exe_path), "%s\\bin\\vecho.exe", g_pier_root);
    snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", exe_path, message);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    if (CreateProcess(exe_path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

static void verror(const char *format, ...)
{
    char msg[MAX_CMD_LEN];
    char *acp;
    char *p;
    va_list args;

    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);

    for (p = msg; *p; p++) {
        if (*p == '"') *p = '\'';
    }

    acp = utf8_to_acp(msg);
    if (acp) {
        vecho_line(acp);
        free(acp);
    } else {
        vecho_line(msg);
    }
}

/* ---- UTF-8 to ACP conversion using Windows API ---- */

static int is_utf8(const char *str, int len)
{
    int i;
    int remaining;

    if (len <= 0) return 0;
    for (i = 0; i < len; i++) {
        if ((unsigned char)str[i] <= 0x7F) {
            continue;
        } else if ((unsigned char)str[i] >= 0xC0 && (unsigned char)str[i] <= 0xDF) {
            remaining = 1;
            i++;
            while (remaining > 0 && i < len) {
                if ((unsigned char)str[i] < 0x80 || (unsigned char)str[i] > 0xBF)
                    return 0;
                remaining--;
                i++;
            }
            if (remaining > 0) return 0;
            i--;
        } else if ((unsigned char)str[i] >= 0xE0 && (unsigned char)str[i] <= 0xEF) {
            remaining = 2;
            i++;
            while (remaining > 0 && i < len) {
                if ((unsigned char)str[i] < 0x80 || (unsigned char)str[i] > 0xBF)
                    return 0;
                remaining--;
                i++;
            }
            if (remaining > 0) return 0;
            i--;
        } else if ((unsigned char)str[i] >= 0xF0 && (unsigned char)str[i] <= 0xF4) {
            remaining = 3;
            i++;
            while (remaining > 0 && i < len) {
                if ((unsigned char)str[i] < 0x80 || (unsigned char)str[i] > 0xBF)
                    return 0;
                remaining--;
                i++;
            }
            if (remaining > 0) return 0;
            i--;
        } else {
            return 0;
        }
    }
    return 1;
}

static char *utf8_to_acp(const char *utf8)
{
    int wlen;
    int alen;
    wchar_t *wbuf;
    char *abuf;
    int len;

    len = (int)strlen(utf8);
    if (len == 0 || !is_utf8(utf8, len)) {
        abuf = (char *)malloc((size_t)len + 1);
        if (abuf) {
            strcpy(abuf, utf8);
        }
        return abuf;
    }

    wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (wlen == 0) {
        abuf = (char *)malloc((size_t)len + 1);
        if (abuf) strcpy(abuf, utf8);
        return abuf;
    }

    wbuf = (wchar_t *)malloc((size_t)wlen * sizeof(wchar_t));
    if (!wbuf) {
        abuf = (char *)malloc((size_t)len + 1);
        if (abuf) strcpy(abuf, utf8);
        return abuf;
    }

    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wbuf, wlen);

    alen = WideCharToMultiByte(CP_ACP, 0, wbuf, -1, NULL, 0, NULL, NULL);
    if (alen == 0) {
        free(wbuf);
        abuf = (char *)malloc((size_t)len + 1);
        if (abuf) strcpy(abuf, utf8);
        return abuf;
    }

    abuf = (char *)malloc((size_t)alen);
    if (!abuf) {
        free(wbuf);
        return NULL;
    }

    WideCharToMultiByte(CP_ACP, 0, wbuf, -1, abuf, alen, NULL, NULL);
    free(wbuf);
    return abuf;
}

static char *trim_line(char *line)
{
    int len;
    char *end;

    while (*line == ' ' || *line == '\t') line++;
    len = (int)strlen(line);
    end = line + len - 1;
    while (end > line && (*end == '\r' || *end == '\n' || *end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    return line;
}

/* ---- INI reader ---- */

static int read_ini_value(const char *inipath, const char *key,
                          char *value, int value_size)
{
    FILE *fp;
    char raw[MAX_LINE];
    int found;

    fp = fopen(inipath, "r");
    if (!fp) return 0;

    found = 0;
    while (fgets(raw, sizeof(raw), fp)) {
        char *line;

        line = trim_line(raw);

        if (line[0] == '[') {
            int i;
            i = 1;
            while (line[i] && line[i] != ']') i++;
            if (line[i] == ']') {
                line[i] = '\0';
                if (strcmp(line + 1, key) == 0) {
                    if (fgets(raw, sizeof(raw), fp)) {
                        char *val_line;

                        val_line = trim_line(raw);

                        strncpy(value, val_line, value_size - 1);
                        value[value_size - 1] = '\0';
                        found = 1;
                    }
                }
            }
        }

        if (found) break;
    }

    fclose(fp);
    return found;
}

/* ---- Error strings initialization ---- */

static void load_error_strings(const char *lang_dir)
{
    char path[MAX_PATH_LEN];
    char value[MAX_ERR_STR];
    int loaded;

    snprintf(path, sizeof(path), "%s\\langsque.ini", lang_dir);
    loaded = 0;

    if (read_ini_value(path, "err_missing_semicolon", value, sizeof(value))) {
        strncpy(err_missing_semicolon, value, sizeof(err_missing_semicolon) - 1);
        loaded++;
    }
    if (read_ini_value(path, "err_single_equal", value, sizeof(value))) {
        strncpy(err_single_equal, value, sizeof(err_single_equal) - 1);
        loaded++;
    }
    if (read_ini_value(path, "err_quote_mismatch", value, sizeof(value))) {
        strncpy(err_quote_mismatch, value, sizeof(err_quote_mismatch) - 1);
        loaded++;
    }
    if (read_ini_value(path, "err_paren_mismatch", value, sizeof(value))) {
        strncpy(err_paren_mismatch, value, sizeof(err_paren_mismatch) - 1);
        loaded++;
    }
    if (read_ini_value(path, "err_undefined_var", value, sizeof(value))) {
        strncpy(err_undefined_var, value, sizeof(err_undefined_var) - 1);
        loaded++;
    }
    if (read_ini_value(path, "err_missing_then", value, sizeof(value))) {
        strncpy(err_missing_then, value, sizeof(err_missing_then) - 1);
        loaded++;
    }
    if (read_ini_value(path, "err_readonly_assign", value, sizeof(value))) {
        strncpy(err_readonly_assign, value, sizeof(err_readonly_assign) - 1);
        loaded++;
    }
    if (read_ini_value(path, "err_not_in_script", value, sizeof(value))) {
        strncpy(err_not_in_script, value, sizeof(err_not_in_script) - 1);
        loaded++;
    }
    if (read_ini_value(path, "msg_no_errors", value, sizeof(value))) {
        strncpy(msg_no_errors, value, sizeof(msg_no_errors) - 1);
        loaded++;
    }

    /* English fallbacks */
    if (!loaded) {
        strncpy(err_missing_semicolon, "Missing semicolon at end of line",
                sizeof(err_missing_semicolon) - 1);
        strncpy(err_single_equal, "Use \"==\" for comparison instead of \"=\"",
                sizeof(err_single_equal) - 1);
        strncpy(err_quote_mismatch, "Quotation mark mismatch or missing",
                sizeof(err_quote_mismatch) - 1);
        strncpy(err_paren_mismatch, "Parenthesis mismatch",
                sizeof(err_paren_mismatch) - 1);
        strncpy(err_undefined_var, "Undefined variable",
                sizeof(err_undefined_var) - 1);
        strncpy(err_missing_then, "Missing \"then\" keyword",
                sizeof(err_missing_then) - 1);
        strncpy(err_readonly_assign, "Cannot assign to read-only variable",
                sizeof(err_readonly_assign) - 1);
        strncpy(err_not_in_script, "\"if\" statement must be inside [Script] section",
                sizeof(err_not_in_script) - 1);
        strncpy(msg_no_errors, "No errors found!",
                sizeof(msg_no_errors) - 1);
    }
}

/* ---- Helpers ---- */

static int is_space(char c)
{
    return c == ' ' || c == '\t';
}

/* Check if line starts with "if " (case-insensitive, ignoring leading whitespace) */
static int starts_with_if(const char *line)
{
    while (*line && (*line == ' ' || *line == '\t')) line++;
    if ((*line == 'i' || *line == 'I') &&
        (*(line+1) == 'f' || *(line+1) == 'F')) {
        return is_space(*(line+2)) ? 1 : 0;
    }
    return 0;
}

/* Find keyword position case-insensitive, returns NULL if not found */
static const char *find_keyword(const char *haystack, const char *keyword)
{
    const char *p;
    int klen;

    klen = (int)strlen(keyword);
    for (p = haystack; *p; p++) {
        if (_strnicmp(p, keyword, klen) == 0) {
            return p;
        }
    }
    return NULL;
}

static int is_known_variable(const char *name, int name_len)
{
    if (name_len < 0) name_len = (int)strlen(name);
    if ((name_len == 9  && _strnicmp(name, "systemver", 9) == 0) ||
        (name_len == 2  && _strnicmp(name, "os", 2) == 0) ||
        (name_len == 7  && _strnicmp(name, "version", 7) == 0) ||
        (name_len == 11 && _strnicmp(name, "packagesize", 11) == 0)) {
        return 1;
    }
    return 0;
}

static int is_readonly_variable(const char *name, int name_len)
{
    if (name_len < 0) name_len = (int)strlen(name);
    return ((name_len == 9 && _strnicmp(name, "systemver", 9) == 0) ||
            (name_len == 2 && _strnicmp(name, "os", 2) == 0));
}

/* Extract snippet around position for error display */
static void get_snippet(const char *line, const char *pos, char *snippet, int size)
{
    int start, len;

    start = (int)(pos - line);
    if (start < 0) start = 0;

    len = (int)strlen(line + start);
    if (len > 60) len = 60;

    strncpy(snippet, line + start, len);
    snippet[len] = '\0';
}

/* ---- Checks ---- */

static int check_semicolon(const char *line, const char *if_start)
{
    const char *end;
    (void)if_start;

    end = line + strlen(line) - 1;
    while (end > line && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
        end--;
    if (end > line && *end != ';') {
        verror("[%d] %s（例如：'%.60s'）",
               g_line_count, err_missing_semicolon, line);
        return 1;
    }
    return 0;
}

static int check_single_equal_in_condition(const char *line, const char *if_start)
{
    const char *then_pos;
    const char *p;
    char work[MAX_LINE];
    int i, len;

    then_pos = find_keyword(if_start + 2, "then");
    if (!then_pos) return 0;

    /* Extract condition part: between "if " and " then" */
    len = (int)(then_pos - (if_start + 2));
    if (len <= 0 || len >= MAX_LINE) return 0;

    strncpy(work, if_start + 2, len);
    work[len] = '\0';

    /* Replace all "==" and "!=" with spaces */
    for (i = 0; work[i]; i++) {
        if (work[i] == '=' && work[i+1] == '=') {
            work[i] = ' ';
            work[i+1] = ' ';
        } else if (work[i] == '!' && work[i+1] == '=') {
            work[i] = ' ';
            work[i+1] = ' ';
        }
    }

    /* Check for remaining '=' that is not a comparison operator */
    for (p = work; *p; p++) {
        if (*p == '=') {
            verror("[%d] %s（例如：'%.60s'）",
                   g_line_count, err_single_equal, line);
            return 1;
        }
    }
    return 0;
}

static int check_quote_match(const char *line, const char *if_start)
{
    const char *p;
    int count;
    (void)if_start;

    count = 0;
    for (p = line; *p; p++) {
        if (*p == '"') count++;
    }

    if (count % 2 != 0) {
        verror("[%d] %s（例如：'%.60s'）",
               g_line_count, err_quote_mismatch, line);
        return 1;
    }
    return 0;
}

static int check_paren_match(const char *line, const char *if_start)
{
    const char *p;
    int count;
    (void)if_start;

    count = 0;
    for (p = line; *p; p++) {
        if (*p == '(') count++;
        else if (*p == ')') count--;
        if (count < 0) {
            verror("[%d] %s（例如：'%.60s'）",
                   g_line_count, err_paren_mismatch, line);
            return 1;
        }
    }
    if (count != 0) {
        verror("[%d] %s（例如：'%.60s'）",
               g_line_count, err_paren_mismatch, line);
        return 1;
    }
    return 0;
}

static int check_undefined_vars(const char *line, const char *if_start)
{
    const char *p;
    (void)if_start;

    for (p = line; *p; p++) {
        if (*p == '{') {
            const char *start;
            int name_len;

            start = p + 1;
            name_len = 0;
            while (start[name_len] && start[name_len] != '}'
                   && name_len < 64) {
                name_len++;
            }

            if (start[name_len] == '}' && name_len > 0) {
                if (!is_known_variable(start, name_len)) {
                    verror("[%d] %s（例如：'{%.*s}'）",
                           g_line_count, err_undefined_var,
                           name_len, start);
                    return 1;
                }
            }
            p = start + name_len;
        }
    }
    return 0;
}

static int check_missing_then(const char *line, const char *if_start)
{
    if (!find_keyword(if_start + 2, "then")) {
        verror("[%d] %s（例如：'%.60s'）",
               g_line_count, err_missing_then, line);
        return 1;
    }
    return 0;
}

static int check_readonly_assign(const char *line, const char *if_start)
{
    const char *then_pos;
    const char *p;
    const char *section;
    int section_len;

    then_pos = find_keyword(if_start + 2, "then");
    if (!then_pos) return 0;

    /* Check then part */
    {
        section = then_pos + 4;
        while (*section && is_space(*section)) section++;

        p = section;
        while (*p && *p != '{') p++;
        if (*p == '{') {
            const char *name_start;
            int name_len;

            name_start = p + 1;
            name_len = 0;
            while (name_start[name_len] && name_start[name_len] != '}'
                   && name_len < 64) {
                name_len++;
            }

            if (is_readonly_variable(name_start, name_len)) {
                verror("[%d] %s（例如：'{%.*s}'）",
                       g_line_count, err_readonly_assign,
                       name_len, name_start);
                return 1;
            }
        }
    }

    /* Check else part if present */
    {
        const char *else_pos;
        const char *semicolon_pos;

        else_pos = find_keyword(then_pos + 4, "else");

        /* Make sure this "else" is between then and ; */
        semicolon_pos = strrchr(then_pos, ';');
        if (else_pos && (!semicolon_pos || else_pos < semicolon_pos)) {
            section = else_pos + 4;
            while (*section && is_space(*section)) section++;

            p = section;
            while (*p && *p != '{') p++;
            if (*p == '{') {
                const char *name_start;
                int name_len;

                name_start = p + 1;
                name_len = 0;
                while (name_start[name_len] && name_start[name_len] != '}'
                       && name_len < 64) {
                    name_len++;
                }

                if (is_readonly_variable(name_start, name_len)) {
                    verror("[%d] %s（例如：'{%.*s}'）",
                           g_line_count, err_readonly_assign,
                           name_len, name_start);
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* ---- Main check function ---- */

static int check_line(const char *raw_line)
{
    char line[MAX_LINE];
    const char *if_start;
    int errs;

    strncpy(line, raw_line, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';

    {
        int llen;
        llen = (int)strlen(line);
        while (llen > 0 && (line[llen-1] == '\r' || line[llen-1] == '\n')) {
            line[--llen] = '\0';
        }
    }

    if (!starts_with_if(line)) return 0;

    if_start = line;
    while (*if_start && is_space(*if_start)) if_start++;

    errs = 0;
    errs += check_semicolon(line, if_start);
    errs += check_single_equal_in_condition(line, if_start);
    errs += check_quote_match(line, if_start);
    errs += check_paren_match(line, if_start);
    errs += check_undefined_vars(line, if_start);
    errs += check_missing_then(line, if_start);
    errs += check_readonly_assign(line, if_start);

    return errs;
}

/* ---- Main ---- */

int main(int argc, char *argv[])
{
    const char *filepath;
    const char *lang_dir;
    FILE *fp;
    char raw_line[MAX_LINE];
    int in_script;

    if (argc < 2) {
        vecho_line("Usage: squecheck.exe <file.sque> <lang_dir>");
        return 1;
    }

    filepath = argv[1];
    lang_dir = (argc >= 3) ? argv[2] : NULL;

    detect_pier_root();

    /* Init error strings */
    err_missing_semicolon[0] = '\0';
    err_single_equal[0] = '\0';
    err_quote_mismatch[0] = '\0';
    err_paren_mismatch[0] = '\0';
    err_undefined_var[0] = '\0';
    err_missing_then[0] = '\0';
    err_readonly_assign[0] = '\0';
    err_not_in_script[0] = '\0';
    msg_no_errors[0] = '\0';

    if (lang_dir) {
        load_error_strings(lang_dir);
    } else {
        load_error_strings(".");
    }

    fp = fopen(filepath, "r");
    if (!fp) {
        vecho_line("Error: Cannot open file");
        return 1;
    }

    g_line_count = 0;
    g_error_count = 0;
    in_script = 0;

    while (fgets(raw_line, sizeof(raw_line), fp)) {
        char cleaned[MAX_LINE];
        int llen;

        g_line_count++;

        strncpy(cleaned, raw_line, sizeof(cleaned) - 1);
        cleaned[sizeof(cleaned) - 1] = '\0';
        llen = (int)strlen(cleaned);
        while (llen > 0 && (cleaned[llen-1] == '\r' || cleaned[llen-1] == '\n')) {
            cleaned[--llen] = '\0';
        }

        /* Track sections: enter/exit [Script] */
        if (cleaned[0] == '[') {
            if (strncmp(cleaned, "[Script]", 8) == 0) {
                in_script = 1;
            } else {
                in_script = 0;
            }
            continue;
        }

        /* Check if-lines only in [Script]; flag them outside */
        if (starts_with_if(cleaned)) {
            if (!in_script) {
                verror("[%d] %s（例如：'%.60s'）",
                       g_line_count, err_not_in_script, cleaned);
                g_error_count++;
            } else {
                g_error_count += check_line(raw_line);
            }
        }
    }

    fclose(fp);

    if (g_error_count == 0) {
        if (msg_no_errors[0]) {
            vecho_line(msg_no_errors);
        } else {
            vecho_line("No errors found!");
        }
        return 0;
    }

    return 1;
}