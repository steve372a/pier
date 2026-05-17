/*
 * pier.c - Main CLI entry point for Pier Package Installer
 * Replaces piec.bat with a native C implementation
 * No cmd.exe, no system(), no sed - pure Windows API
 * Compatible with C89/C90, Windows XP and later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "sque.h"

#define MAX_PATH_LEN 1024
#define MAX_LINE 4096
#define MAX_NAME 256
#define MAX_LANG_STRINGS 512
#define MAX_CMD_LEN 8192
#define VERSION "2.4.0"

/* ============================================================
 * Language string hash table entry
 * ============================================================ */
typedef struct {
    char key[MAX_NAME];
    char value[MAX_LINE];
} lang_entry_t;

/* ============================================================
 * Global state
 * ============================================================ */
char g_pier_root[MAX_PATH_LEN];
char g_language_dir[MAX_PATH_LEN];
char g_source_url[MAX_PATH_LEN];
char g_pies_url[MAX_PATH_LEN];
char g_sys_arch[MAX_NAME];
char g_current_lang[MAX_NAME] = "zh-CN";
int g_autoyes = 0;

lang_entry_t g_lang_table[MAX_LANG_STRINGS];
int g_lang_count = 0;

/* ============================================================
 * Function prototypes
 * ============================================================ */
void detect_pier_root(void);
int load_language_strings(const char *lang_dir);
const char *get_lang(const char *key);
void vecho_line(const char *format, ...);
int confirm_prompt(const char *message);
int execute_tool(const char *tool_name, const char *args);

/* Command handlers */
int cmd_install(int argc, char *argv[]);
int cmd_remove(int argc, char *argv[]);
int cmd_search(int argc, char *argv[]);
int cmd_list(int argc, char *argv[]);
int cmd_open(int argc, char *argv[]);
int cmd_sources(int argc, char *argv[]);
int cmd_setlang(int argc, char *argv[]);
int cmd_updpath(int argc, char *argv[]);
int cmd_delpath(int argc, char *argv[]);
int cmd_help(int argc, char *argv[]);

/* ============================================================
 * Detect PIER_ROOT from executable path
 * pier.exe is in root directory (not bin/)
 * ============================================================ */
void detect_pier_root(void) {
    char exe_path[MAX_PATH_LEN];
    char *last_slash;
    DWORD len;

    len = GetModuleFileName(NULL, exe_path, sizeof(exe_path));
    if (len == 0 || len >= sizeof(exe_path)) {
        GetCurrentDirectory(sizeof(g_pier_root), g_pier_root);
        return;
    }

    last_slash = strrchr(exe_path, '\\');
    if (last_slash) {
        *last_slash = '\0';
    }

    strncpy(g_pier_root, exe_path, sizeof(g_pier_root) - 1);
    g_pier_root[sizeof(g_pier_root) - 1] = '\0';
}

/* ============================================================
 * Load all language strings from lang.ini into hash table
 * Replaces: 30+ sed calls
 * ============================================================ */
int load_language_strings(const char *lang_dir) {
    FILE *fp;
    char filepath[MAX_PATH_LEN];
    char line[MAX_LINE];
    char current_key[MAX_NAME];
    int in_field = 0;

    snprintf(filepath, sizeof(filepath), "%s\\lang.ini", lang_dir);

    fp = fopen(filepath, "r");
    if (!fp) {
        return -1;
    }

    g_lang_count = 0;
    current_key[0] = '\0';

    while (fgets(line, sizeof(line), fp) != NULL && g_lang_count < MAX_LANG_STRINGS) {
        char *p = line;
        while (*p) {
            if (*p == '\r' || *p == '\n') {
                *p = '\0';
                break;
            }
            p++;
        }

        if (strlen(line) == 0) {
            in_field = 0;
            continue;
        }

        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (end) {
                *end = '\0';
                strncpy(current_key, line + 1, sizeof(current_key) - 1);
                current_key[sizeof(current_key) - 1] = '\0';
                in_field = 1;
            }
            continue;
        }

        if (in_field && current_key[0] != '\0') {
            strncpy(g_lang_table[g_lang_count].key, current_key, MAX_NAME - 1);
            g_lang_table[g_lang_count].key[MAX_NAME - 1] = '\0';
            strncpy(g_lang_table[g_lang_count].value, line, MAX_LINE - 1);
            g_lang_table[g_lang_count].value[MAX_LINE - 1] = '\0';
            g_lang_count++;
            in_field = 0;
        }
    }

    fclose(fp);
    return 0;
}

/* ============================================================
 * Get language string by key
 * ============================================================ */
const char *get_lang(const char *key) {
    int i;
    for (i = 0; i < g_lang_count; i++) {
        if (strcmp(g_lang_table[i].key, key) == 0) {
            return g_lang_table[i].value;
        }
    }
    return key;
}

/* ============================================================
 * Call vecho.exe via CreateProcess for colored output
 * ============================================================ */
void vecho_line(const char *format, ...) {
    char exe_path[MAX_PATH_LEN];
    char cmdline[MAX_PATH_LEN * 4];
    char message[MAX_LINE];
    va_list args;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

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

/* ============================================================
 * Confirm prompt with Y/N
 * ============================================================ */
int confirm_prompt(const char *message) {
    char input[16];

    if (g_autoyes) {
        return 1;
    }

    printf("%s (Y/N): ", message);
    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (input[0] == 'Y' || input[0] == 'y') {
            return 1;
        }
    }
    return 0;
}

/* ============================================================
 * Execute a tool using CreateProcess (no cmd.exe, no system())
 * Passes custom environment block with bin/ in PATH
 * Does NOT modify current process PATH
 * ============================================================ */
int execute_tool(const char *tool_name, const char *args) {
    char exe_path[MAX_PATH_LEN];
    char cmdline[MAX_CMD_LEN];
    char *env_block = NULL;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD exit_code;
    int ret;

    /* Build executable path */
    snprintf(exe_path, sizeof(exe_path), "%s\\bin\\%s.exe", g_pier_root, tool_name);

    /* Build command line: "exe_path" args */
    snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", exe_path, args);

    /* Build custom environment block with bin/ added to PATH */
    {
        char *env;
        int env_size;
        int found_path = 0;
        int i;

        /* Get current environment block */
        env = (char*)GetEnvironmentStringsA();
        if (!env) {
            printf("Failed to get environment\n");
            return 1;
        }

        /* Calculate current environment size */
        env_size = 0;
        for (i = 0; env[i] != '\0'; ) {
            env_size += strlen(&env[i]) + 1; /* +1 for null terminator */
            i += strlen(&env[i]) + 1;
        }
        env_size += 2; /* double null terminator */

        /* Calculate new environment size (rough estimate) */
        env_size += MAX_PATH_LEN + 10; /* extra for bin\; prefix */

        /* Allocate and build new environment block */
        env_block = (char*)malloc(env_size + 2); /* +2 for double null terminator */
        if (!env_block) {
            FreeEnvironmentStringsA(env);
            return 1;
        }

        memset(env_block, 0, env_size + 2);

        /* Copy environment variables, modifying PATH */
        {
            char *dst = env_block;
            for (i = 0; env[i] != '\0'; ) {
                int var_len = strlen(&env[i]);

                if (_strnicmp(&env[i], "PATH=", 5) == 0) {
                    /* Modify PATH: prepend bin directory */
                    snprintf(dst, env_size - (int)(dst - env_block),
                             "PATH=%s\\bin;%s", g_pier_root, &env[i] + 5);
                    found_path = 1;
                } else {
                    strcpy(dst, &env[i]);
                }

                dst += strlen(dst) + 1;
                i += var_len + 1;
            }

            /* If no PATH found, add one */
            if (!found_path) {
                snprintf(dst, env_size - (int)(dst - env_block),
                         "PATH=%s\\bin", g_pier_root);
                dst += strlen(dst) + 1;
            }
            *dst = '\0'; /* Double null terminator */
        }

        FreeEnvironmentStringsA(env);
    }

    /* Initialize structures */
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    /* Inherit stdout/stderr so subprograms can show output and read input */
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    memset(&pi, 0, sizeof(pi));

    /* Create process with custom environment */
    ret = CreateProcess(
        exe_path,       /* Application name */
        cmdline,        /* Command line */
        NULL,           /* Process security attributes */
        NULL,           /* Thread security attributes */
        TRUE,           /* Inherit handles (for std handles) */
        0,              /* Creation flags */
        env_block,      /* Custom environment with modified PATH */
        g_pier_root,    /* Current directory */
        &si,            /* Startup info */
        &pi             /* Process info */
    );

    /* Free environment block immediately after CreateProcess */
    free(env_block);

    if (!ret) {
        printf("Failed to start %s.exe (error: %lu)\n", tool_name, GetLastError());
        return 1;
    }

    /* Wait for process to complete */
    WaitForSingleObject(pi.hProcess, INFINITE);

    /* Get exit code */
    GetExitCodeProcess(pi.hProcess, &exit_code);

    /* Cleanup */
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (int)exit_code;
}

/* ============================================================
 * Command: install
 * ============================================================ */
int cmd_install(int argc, char *argv[]) {
    char args[MAX_CMD_LEN];
    char full_source_url[MAX_PATH_LEN];
    char full_pies_url[MAX_PATH_LEN];
    int i, pos;

    if (argc < 3) {
        printf("%s\n", get_lang("error_no_package"));
        return 2;
    }

    snprintf(full_source_url, sizeof(full_source_url), "%s/sources", g_source_url);
    snprintf(full_pies_url, sizeof(full_pies_url), "%s", g_pies_url);

    pos = snprintf(args, sizeof(args),
             "install \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
             g_pier_root, g_language_dir, full_source_url, full_pies_url,
             g_autoyes ? "y" : "n", g_sys_arch);

    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--yes") == 0) continue;
        if (pos < sizeof(args) - 1) {
            int w = snprintf(args + pos, sizeof(args) - pos, " \"%s\"", argv[i]);
            if (w > 0 && w < sizeof(args) - pos) pos += w;
            else pos = (int)sizeof(args) - 1;
        }
    }
    return execute_tool("pier-pkg", args);
}

/* ============================================================
 * Command: remove
 * ============================================================ */
int cmd_remove(int argc, char *argv[]) {
    char args[MAX_CMD_LEN];
    char full_source_url[MAX_PATH_LEN];
    int i, pos;

    if (argc < 3) {
        printf("%s\n", get_lang("error_no_package_remove"));
        return 2;
    }

    snprintf(full_source_url, sizeof(full_source_url), "%s/sources", g_source_url);

    pos = snprintf(args, sizeof(args),
             "remove \"%s\" \"%s\" \"%s\" \"%s\"",
             g_pier_root, g_language_dir, full_source_url, g_autoyes ? "y" : "n");

    for (i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--yes") == 0) continue;
        if (pos < sizeof(args) - 1) {
            int w = snprintf(args + pos, sizeof(args) - pos, " \"%s\"", argv[i]);
            if (w > 0 && w < sizeof(args) - pos) pos += w;
            else pos = (int)sizeof(args) - 1;
        }
    }
    return execute_tool("pier-pkg", args);
}

/* ============================================================
 * Command: search
 * ============================================================ */
int cmd_search(int argc, char *argv[]) {
    char args[MAX_CMD_LEN];

    if (argc < 3) {
        printf("%s\n", get_lang("error_no_package"));
        return 2;
    }

    snprintf(args, sizeof(args),
             "search \"%s\" \"%s\" \"%s\" \"%s\"",
             g_pier_root, g_language_dir, g_source_url, argv[2]);

    return execute_tool("pier-pkg", args);
}

/* ============================================================
 * Command: list
 * ============================================================ */
int cmd_list(int argc, char *argv[]) {
    printf("List command - downloading package list...\n");
    return 0;
}

/* ============================================================
 * Command: open (o)
 * ============================================================ */
int cmd_open(int argc, char *argv[]) {
    char args[MAX_CMD_LEN];
    char full_source_url[MAX_PATH_LEN];
    int i, pos;

    if (argc < 3) {
        printf("%s\n", get_lang("error_no_package"));
        return 2;
    }

    snprintf(full_source_url, sizeof(full_source_url), "%s/sources", g_source_url);

    pos = snprintf(args, sizeof(args), "\"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
             g_pier_root, g_language_dir, g_source_url, full_source_url, argv[2]);

    for (i = 3; i < argc; i++) {
        if (pos < sizeof(args) - 1) {
            int w = snprintf(args + pos, sizeof(args) - pos, " \"%s\"", argv[i]);
            if (w > 0 && w < sizeof(args) - pos) pos += w;
            else pos = (int)sizeof(args) - 1;
        }
    }
    return execute_tool("pier-op", args);
}

/* ============================================================
 * Command: sque
 * ============================================================ */
int cmd_sque(int argc, char *argv[]) {
    char args[MAX_PATH_LEN * 3];
    char sque_file[MAX_PATH_LEN * 2];
    int pos;
    int i;

    if (argc < 3) {
        printf("Usage: pier sque -c <file.sque>\n");
        printf("       pier sque check <file.sque>\n");
        return 0;
    }

    if (strcmp(argv[2], "-c") == 0 || strcmp(argv[2], "check") == 0) {
        if (argc < 4) {
            printf("Error: missing file argument\n");
            printf("Usage: pier sque -c <file.sque>\n");
            return 2;
        }

        /* Build full path for file if relative */
        if (strchr(argv[3], ':') == NULL && argv[3][0] != '\\') {
            snprintf(sque_file, sizeof(sque_file), "%s\\%s", g_pier_root, argv[3]);
        } else {
            strncpy(sque_file, argv[3], sizeof(sque_file) - 1);
            sque_file[sizeof(sque_file) - 1] = '\0';
        }

        pos = (int) snprintf(args, sizeof(args), "\"%s\" \"%s\"",
                             sque_file, g_language_dir);

        for (i = 4; i < argc; i++) {
            if (pos < (int)sizeof(args) - 1) {
                int w = snprintf(args + pos, sizeof(args) - pos, " \"%s\"", argv[i]);
                if (w > 0 && w < (int)sizeof(args) - pos) pos += w;
                else pos = (int)sizeof(args) - 1;
            }
        }

        return execute_tool("squecheck", args);
    }

    printf("Usage: pier sque -c <file.sque>\n");
    printf("       pier sque check <file.sque>\n");
    return 2;
}

/* ============================================================
 * Command: hash
 * ============================================================ */
int cmd_hash(int argc, char *argv[]) {
    char args[MAX_PATH_LEN * 4];
    char file1[MAX_PATH_LEN * 2];
    char file2[MAX_PATH_LEN * 2];
    int pos;
    int i;
    int is_check;

    if (argc < 4) {
        printf("Usage: pier hash -c <file1> <file2>\n");
        printf("       pier hash check <file1> <file2>\n");
        printf("       pier hash -g <file>\n");
        printf("       pier hash gene <file>\n");
        return 0;
    }

    is_check = (strcmp(argv[2], "-c") == 0 || strcmp(argv[2], "check") == 0);

    if (is_check) {
        if (argc < 5) {
            printf("Error: missing file arguments\n");
            printf("Usage: pier hash -c <file1> <file2>\n");
            return 2;
        }

        if (strchr(argv[3], ':') == NULL && argv[3][0] != '\\') {
            snprintf(file1, sizeof(file1), "%s\\%s", g_pier_root, argv[3]);
        } else {
            strncpy(file1, argv[3], sizeof(file1) - 1);
            file1[sizeof(file1) - 1] = '\0';
        }

        if (strchr(argv[4], ':') == NULL && argv[4][0] != '\\') {
            snprintf(file2, sizeof(file2), "%s\\%s", g_pier_root, argv[4]);
        } else {
            strncpy(file2, argv[4], sizeof(file2) - 1);
            file2[sizeof(file2) - 1] = '\0';
        }

        pos = snprintf(args, sizeof(args), "%s \"%s\" \"%s\"",
                       argv[2], file1, file2);
    } else if (strcmp(argv[2], "-g") == 0 || strcmp(argv[2], "gene") == 0) {
        if (argc < 4) {
            printf("Error: missing file argument\n");
            printf("Usage: pier hash -g <file>\n");
            return 2;
        }

        if (strchr(argv[3], ':') == NULL && argv[3][0] != '\\') {
            snprintf(file1, sizeof(file1), "%s\\%s", g_pier_root, argv[3]);
        } else {
            strncpy(file1, argv[3], sizeof(file1) - 1);
            file1[sizeof(file1) - 1] = '\0';
        }

        pos = snprintf(args, sizeof(args), "%s \"%s\"",
                       argv[2], file1);
    } else {
        printf("Usage: pier hash -c <file1> <file2>\n");
        printf("       pier hash check <file1> <file2>\n");
        printf("       pier hash -g <file>\n");
        printf("       pier hash gene <file>\n");
        return 2;
    }

    for (i = (is_check ? 5 : 4); i < argc; i++) {
        if (pos < (int)sizeof(args) - 1) {
            int w = snprintf(args + pos, sizeof(args) - pos, " \"%s\"", argv[i]);
            if (w > 0 && w < (int)sizeof(args) - pos) pos += w;
            else pos = (int)sizeof(args) - 1;
        }
    }

    {
        int w = snprintf(args + pos, sizeof(args) - pos, " --lang \"%s\"", g_language_dir);
        if (w > 0 && w < (int)sizeof(args) - pos) pos += w;
    }

    return execute_tool("pier-hash", args);
}

/* ============================================================
 * Command: sources
 * ============================================================ */
int cmd_sources(int argc, char *argv[]) {
    if (argc < 3 || strcmp(argv[2], "list") == 0) {
        printf("Source URL: %s\n", g_source_url);
        printf("Pie URL: %s\n", g_pies_url);
        return 0;
    }

    if (strcmp(argv[2], "change") == 0 || strcmp(argv[2], "chg") == 0) {
        printf("Source change not yet implemented\n");
        return 0;
    }

    printf("%s\n", get_lang("error_invalid_cmd"));
    return 2;
}

/* ============================================================
 * Command: setlang
 * ============================================================ */
int cmd_setlang(int argc, char *argv[]) {
    if (argc < 3) {
        printf("%s\n", get_lang("error_no_param"));
        return 2;
    }

    if (strcmp(argv[2], "set") == 0 && argc >= 4) {
        char lang_path[MAX_PATH_LEN];
        char ini_path[MAX_PATH_LEN];
        FILE *fp;

        snprintf(lang_path, sizeof(lang_path), "%%PIER_ROOT%%\\share\\language\\%s", argv[3]);
        snprintf(ini_path, sizeof(ini_path), "%s\\etc\\language.ini", g_pier_root);

        fp = fopen(ini_path, "w");
        if (fp) {
            fprintf(fp, "%s", lang_path);
            fclose(fp);
            printf("%s\n", get_lang("language_set_success"));
            return 0;
        }
        return 1;
    }

    printf("%s\n", get_lang("error_invalid_cmd"));
    return 2;
}

/* ============================================================
 * Command: updpath
 * ============================================================ */
int cmd_updpath(int argc, char *argv[]) {
    HKEY hKey;
    char path_buf[8192];
    DWORD path_len = sizeof(path_buf);
    LONG result;
    char *scuts_pos;
    char *seg_start;
    char *seg_end;
    char old_seg[MAX_PATH_LEN];
    char new_seg[MAX_PATH_LEN];
    char new_buf[8192];
    char exe_path[MAX_PATH_LEN];
    char *last_slash;
    int prefix_len;
    int old_len;
    int new_len;
    int rest_len;

    (void)argc;
    (void)argv;

    result = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) {
        vecho_line("$white$%s", get_lang("updpath_not_found"));
        return 1;
    }

    path_buf[0] = '\0';
    result = RegQueryValueExA(hKey, "PATH", NULL, NULL, (BYTE *)path_buf, &path_len);
    if (result != ERROR_SUCCESS || path_len <= 1) {
        RegCloseKey(hKey);
        vecho_line("$white$%s", get_lang("updpath_not_found"));
        return 1;
    }

    scuts_pos = strstr(path_buf, "\\scuts");
    if (!scuts_pos) {
        char answer[8];
        vecho_line("$brightyellow$%s", get_lang("updpath_add_prompt"));
        printf("> ");
        fflush(stdout);
        if (fgets(answer, sizeof(answer), stdin)) {
            if (answer[0] == 'y' || answer[0] == 'Y') {
                GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
                last_slash = strrchr(exe_path, '\\');
                if (last_slash) *last_slash = '\0';

                if (strlen(path_buf) > 0 && path_buf[strlen(path_buf) - 1] != ';') {
                    strcat(path_buf, ";");
                }
                snprintf(path_buf + strlen(path_buf),
                         sizeof(path_buf) - strlen(path_buf),
                         "%s\\scuts", exe_path);

                result = RegSetValueExA(hKey, "PATH", 0, REG_EXPAND_SZ,
                                        (BYTE *)path_buf,
                                        (DWORD)(strlen(path_buf) + 1));
                RegCloseKey(hKey);
                if (result == ERROR_SUCCESS) {
                    vecho_line("$brightcyan$%s", get_lang("updpath_add_yes"));
                    return 0;
                }
                vecho_line("$brightred$%s", get_lang("updpath_not_found"));
                return 1;
            }
            vecho_line("$white$%s", get_lang("updpath_add_no"));
        }
        RegCloseKey(hKey);
        return 1;
    }

    /* Find the start of this PATH segment (go back to previous ';' or beginning) */
    seg_end = strchr(scuts_pos, ';');
    seg_start = scuts_pos;
    while (seg_start > path_buf && *(seg_start - 1) != ';') {
        seg_start--;
    }
    seg_end = seg_end ? seg_end : scuts_pos + strlen(scuts_pos);

    /* Extract old segment */
    old_len = (int)(seg_end - seg_start);
    if (old_len >= (int)sizeof(old_seg)) old_len = (int)sizeof(old_seg) - 1;
    memcpy(old_seg, seg_start, old_len);
    old_seg[old_len] = '\0';

    /* Get current pier root */
    GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    last_slash = strrchr(exe_path, '\\');
    if (last_slash) *last_slash = '\0';
    snprintf(new_seg, sizeof(new_seg), "%s\\scuts", exe_path);

    /* Build new PATH string */
    prefix_len = (int)(seg_start - path_buf);
    memcpy(new_buf, path_buf, prefix_len);
    new_len = (int)strlen(new_seg);
    memcpy(new_buf + prefix_len, new_seg, new_len);
    rest_len = (int)strlen(seg_end);
    memcpy(new_buf + prefix_len + new_len, seg_end, rest_len + 1);

    /* Write back to registry */
    result = RegSetValueExA(hKey, "PATH", 0, REG_EXPAND_SZ, (BYTE *)new_buf,
                            (DWORD)(strlen(new_buf) + 1));
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        vecho_line("$brightred$%s", get_lang("updpath_not_found"));
        return 1;
    }

    /* Registry already written via RegSetValueExA */
    
    vecho_line("$brightcyan$%s", get_lang("updpath_done"));
    return 0;
}

/* ============================================================
 * Command: delpath
 * Remove pier's scuts directory from permanent PATH
 * ============================================================ */
int cmd_delpath(int argc, char *argv[]) {
    HKEY hKey;
    char path_buf[8192];
    DWORD path_len = sizeof(path_buf);
    LONG result;
    char *scuts_pos;
    char *seg_start;
    char *seg_end;
    char new_buf[8192];
    int prefix_len;
    int seg_len;

    (void)argc;
    (void)argv;

    result = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) {
        vecho_line("$white$%s", get_lang("delpath_not_found"));
        return 1;
    }

    path_buf[0] = '\0';
    result = RegQueryValueExA(hKey, "PATH", NULL, NULL, (BYTE *)path_buf, &path_len);
    if (result != ERROR_SUCCESS || path_len <= 1) {
        RegCloseKey(hKey);
        vecho_line("$white$%s", get_lang("delpath_not_found"));
        return 1;
    }

    scuts_pos = strstr(path_buf, "\\scuts");
    if (!scuts_pos) {
        RegCloseKey(hKey);
        vecho_line("$white$%s", get_lang("delpath_not_found"));
        return 1;
    }

    seg_start = scuts_pos;
    while (seg_start > path_buf && *(seg_start - 1) != ';') {
        seg_start--;
    }
    seg_end = strchr(scuts_pos, ';');
    seg_end = seg_end ? seg_end : scuts_pos + strlen(scuts_pos);

    prefix_len = (int)(seg_start - path_buf);
    memcpy(new_buf, path_buf, prefix_len);

    if (*seg_end == ';') seg_end++;
    strcpy(new_buf + prefix_len, seg_end);

    seg_len = (int)strlen(new_buf);
    while (seg_len > 0 && new_buf[seg_len - 1] == ';') {
        new_buf[--seg_len] = '\0';
    }

    result = RegSetValueExA(hKey, "PATH", 0, REG_EXPAND_SZ, (BYTE *)new_buf,
                            (DWORD)(strlen(new_buf) + 1));
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        vecho_line("$brightred$%s", get_lang("delpath_not_found"));
        return 1;
    }

    vecho_line("$brightcyan$%s", get_lang("delpath_done"));
    return 0;
}

/* ============================================================
 * Command: help
 * ============================================================ */
int cmd_help(int argc, char *argv[]) {
    char help_file[MAX_PATH_LEN];
    FILE *fp;
    char line[MAX_LINE];

    snprintf(help_file, sizeof(help_file), "%s\\help.lang", g_language_dir);

    fp = fopen(help_file, "r");
    if (fp) {
        while (fgets(line, sizeof(line), fp) != NULL) {
            printf("%s", line);
        }
        fclose(fp);
    } else {
        printf("Pier Package Installer v%s\n", VERSION);
        printf("Usage: pier <command> [options] [package]\n\n");
        printf("Commands:\n");
        printf("  install <package> [-y]     Install a package\n");
        printf("  remove <package> [-y]      Remove a package\n");
        printf("  search <keyword>           Search for packages\n");
        printf("  list                       List available packages\n");
        printf("  o <package> [args...]      Open/run a package\n");
        printf("  sources [change|list]      Manage sources\n");
        printf("  setlang set <lang>         Set language\n");
        printf("  help                       Show this help\n");
    }

    return 0;
}

/* ============================================================
 * Get system architecture
 * Replaces: pier-arch.exe sysarch
 * ============================================================ */
void get_system_arch(void) {
    SYSTEM_INFO si;
    typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
    PGNSI pGNSI = (PGNSI)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetNativeSystemInfo");

    if (pGNSI) {
        pGNSI(&si);
    } else {
        GetSystemInfo(&si);
    }

    switch (si.wProcessorArchitecture) {
        case 9:  /* PROCESSOR_ARCHITECTURE_AMD64 */
            strcpy(g_sys_arch, "x64");
            break;
        case 0:  /* PROCESSOR_ARCHITECTURE_INTEL */
            strcpy(g_sys_arch, "x86");
            break;
        case 5:  /* PROCESSOR_ARCHITECTURE_ARM */
            strcpy(g_sys_arch, "arm");
            break;
        case 12: /* PROCESSOR_ARCHITECTURE_ARM64 */
            strcpy(g_sys_arch, "arm64");
            break;
        default:
            strcpy(g_sys_arch, "x86");
            break;
    }
}

/* ============================================================
 * Main entry point
 * ============================================================ */
int main(int argc, char *argv[]) {
    char lang_ini_path[MAX_PATH_LEN];
    char sourceimage_path[MAX_PATH_LEN];
    int ret;

    /* Detect PIER_ROOT from executable location */
    detect_pier_root();

    /* Read language directory from etc/language.ini (plain text, not INI format) */
    snprintf(lang_ini_path, sizeof(lang_ini_path), "%s\\etc\\language.ini", g_pier_root);
    {
        FILE *fp = fopen(lang_ini_path, "r");
        if (fp) {
            if (fgets(g_language_dir, sizeof(g_language_dir), fp) != NULL) {
                char *p = g_language_dir;
                while (*p) {
                    if (*p == '\r' || *p == '\n') {
                        *p = '\0';
                        break;
                    }
                    p++;
                }
            }
            fclose(fp);
        }
    }

    /* Replace %PIER_ROOT% placeholder */
    {
        char *p = strstr(g_language_dir, "%PIER_ROOT%");
        if (p != NULL) {
            char temp[MAX_PATH_LEN];
            *p = '\0';
            snprintf(temp, sizeof(temp), "%s%s%s",
                     g_language_dir, g_pier_root, p + 11);
            strncpy(g_language_dir, temp, sizeof(g_language_dir) - 1);
            g_language_dir[sizeof(g_language_dir) - 1] = '\0';
        }
    }

    /* Fallback to zh-CN if lang.ini not found */
    snprintf(lang_ini_path, sizeof(lang_ini_path), "%s\\lang.ini", g_language_dir);
    if (GetFileAttributes(lang_ini_path) == INVALID_FILE_ATTRIBUTES) {
        snprintf(g_language_dir, sizeof(g_language_dir), "%s\\share\\language\\zh-CN", g_pier_root);
    }

    /* Load all language strings */
    load_language_strings(g_language_dir);

    /* Read source URLs from sourceimage.ini */
    snprintf(sourceimage_path, sizeof(sourceimage_path), "%s\\etc\\sourceimage.ini", g_pier_root);
    sque_read(sourceimage_path, "package_source", g_source_url, sizeof(g_source_url));
    sque_read(sourceimage_path, "pie_source", g_pies_url, sizeof(g_pies_url));

    /* Get system architecture (native API, no external tool) */
    get_system_arch();

    /* Check for -y flag */
    {
        int i;
        for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--yes") == 0) {
                g_autoyes = 1;
                break;
            }
        }
    }

    /* Output welcome message */
    {
        const char *welcome = get_lang("welcome");
        if (welcome && welcome[0] != '\0' && strcmp(welcome, "welcome") != 0) {
            printf("%s ", welcome);
            vecho_line("$brightcyan$%s", VERSION);
        }
    }

    /* Route command */
    if (argc < 2) {
        printf("%s\n", get_lang("error_no_param"));
        return 2;
    }

    if (strcmp(argv[1], "install") == 0) {
        ret = cmd_install(argc, argv);
    } else if (strcmp(argv[1], "remove") == 0) {
        ret = cmd_remove(argc, argv);
    } else if (strcmp(argv[1], "search") == 0) {
        ret = cmd_search(argc, argv);
    } else if (strcmp(argv[1], "list") == 0) {
        ret = cmd_list(argc, argv);
    } else if (strcmp(argv[1], "o") == 0) {
        ret = cmd_open(argc, argv);
    } else if (strcmp(argv[1], "sources") == 0) {
        ret = cmd_sources(argc, argv);
    } else if (strcmp(argv[1], "sque") == 0) {
        ret = cmd_sque(argc, argv);
    } else if (strcmp(argv[1], "hash") == 0) {
        ret = cmd_hash(argc, argv);
    } else if (strcmp(argv[1], "setlang") == 0 || strcmp(argv[1], "sl") == 0) {
        ret = cmd_setlang(argc, argv);
    } else if (strcmp(argv[1], "updpath") == 0) {
        ret = cmd_updpath(argc, argv);
    } else if (strcmp(argv[1], "delpath") == 0) {
        ret = cmd_delpath(argc, argv);
    } else if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 ||
               strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "?") == 0) {
        ret = cmd_help(argc, argv);
    } else {
        printf("%s\n", get_lang("error_invalid_cmd"));
        ret = 2;
    }

    return ret;
}
