/*
 * pier-op.c - Package open command handler for Pier
 * Compatible with C89/C90, Windows XP and later
 * Sanakaprix (https://steve372a.github.io)
 *
 * Usage: pier-op.exe <PIER_ROOT> <LANGUAGE_DIR> <source> <full_source_url> <package> [alias] [args...]
 *   PIER_ROOT         - Pier installation root path
 *   LANGUAGE_DIR      - Language file directory
 *   source            - Software source URL
 *   full_source_url   - Full source URL (source + /sources)
 *   package           - Package name or "user/package" format
 *   alias             - Optional alias name
 *   args...           - Additional arguments (unlimited)
 *
 * Note: alias_source is hardcoded to https://steve372a.github.io/pier-repo
 *       and cannot be changed by user.
 *
 * Return codes:
 *   0 - Success
 *   1 - Error (metadata not found, file not found, etc.)
 *   2 - Package not installed (for third-party alias)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "sque.h"

#define MAX_LINE 1024
#define MAX_PATH_LEN 512
#define MAX_ARGS 9999
#define MAX_CMD_LEN 8192

/* Hardcoded alias source URL - cannot be changed by user */
#define ALIAS_SOURCE "https://steve372a.github.io/pier-repo"

/* Function prototypes */
int parse_arguments(int argc, char *argv[], char **pier_root, char **lang_dir, char **source, char **full_source_url, char **package, char **alias, int *user_arg_start);
int is_third_party_alias(const char *package);
int download_alias_template(const char *lang_dir, const char *source, const char *package, const char *cache_dir, char *alias_file, int alias_file_size);
int download_metadata(const char *lang_dir, const char *full_source_url, const char *package_name, const char *cache_dir);
int parse_alias_touse(const char *alias_file, char *package_name, int package_name_size);
int extract_alias_section(const char *alias_file, const char *alias_name, char *program, int program_size);
int extract_defaultopen(const char *metadata_file, const char *installdir, const char *pier_root, char *program_path, int program_path_size);
void build_and_execute(const char *program_path, const char *alias_template, int argc, char *argv[], int user_arg_start);
int run_vecho(const char *pier_root, const char *message);
void replace_placeholders(const char *template, char **user_args, int user_arg_count, int *used_args, char *output, int output_size);
void trim_whitespace(char *str);
void extract_filename(char *str);
int starts_with(const char *str, const char *prefix);
int strcmpi(const char *s1, const char *s2);
void url_encode(const char *input, char *output, int output_size);

/* Global for language strings */
char g_lang_file[MAX_PATH_LEN];
char g_loading_metadata[MAX_LINE];
char g_error_alias_not_found[MAX_LINE];
char g_open_available_aliases[MAX_LINE];
char g_open_program[MAX_LINE];
char g_open_metadata_error[MAX_LINE];
char g_open_package_not_found[MAX_LINE];
char g_open_file_not_found[MAX_LINE];
char g_error_no_default_open[MAX_LINE];
char g_open_alias_not_installed[MAX_LINE];
char g_open_alias_install_hint[MAX_LINE];
char g_open_alias_parse_error[MAX_LINE];
char g_open_alias_not_found_template[MAX_LINE];
char g_open_alias_touse_error[MAX_LINE];
char g_open_alias_syntax_error[MAX_LINE];

void build_wget_proxy_opts(char *buf, int buf_size) {
    char *http_proxy, *https_proxy;
    buf[0] = '\0';
    http_proxy = getenv("http_proxy");
    if (http_proxy && http_proxy[0]) {
        snprintf(buf + strlen(buf), buf_size - strlen(buf), " -e use_proxy=on -e http_proxy=%s", http_proxy);
    }
    https_proxy = getenv("https_proxy");
    if (https_proxy && https_proxy[0]) {
        if (buf[0] == '\0') {
            snprintf(buf + strlen(buf), buf_size - strlen(buf), " -e use_proxy=on");
        }
        snprintf(buf + strlen(buf), buf_size - strlen(buf), " -e https_proxy=%s", https_proxy);
    }
}

/* Read language string from lang.ini using sque library */
void read_lang_string(const char *fieldname, char *output, int output_size) {
    if (sque_read(g_lang_file, fieldname, output, output_size) < 0) {
        output[0] = '\0';
    }
}

/* Initialize language strings */
void init_lang_strings(void) {
    read_lang_string("loading_metadata", g_loading_metadata, sizeof(g_loading_metadata));
    read_lang_string("error_alias_not_found", g_error_alias_not_found, sizeof(g_error_alias_not_found));
    read_lang_string("open_available_aliases", g_open_available_aliases, sizeof(g_open_available_aliases));
    read_lang_string("open_program", g_open_program, sizeof(g_open_program));
    read_lang_string("open_metadata_error", g_open_metadata_error, sizeof(g_open_metadata_error));
    read_lang_string("open_package_not_found", g_open_package_not_found, sizeof(g_open_package_not_found));
    read_lang_string("open_file_not_found", g_open_file_not_found, sizeof(g_open_file_not_found));
    read_lang_string("error_no_default_open", g_error_no_default_open, sizeof(g_error_no_default_open));
    read_lang_string("open_alias_not_installed", g_open_alias_not_installed, sizeof(g_open_alias_not_installed));
    read_lang_string("open_alias_install_hint", g_open_alias_install_hint, sizeof(g_open_alias_install_hint));
    read_lang_string("open_alias_parse_error", g_open_alias_parse_error, sizeof(g_open_alias_parse_error));
    read_lang_string("open_alias_not_found", g_open_alias_not_found_template, sizeof(g_open_alias_not_found_template));
    read_lang_string("open_alias_touse_error", g_open_alias_touse_error, sizeof(g_open_alias_touse_error));
    read_lang_string("open_alias_syntax_error", g_open_alias_syntax_error, sizeof(g_open_alias_syntax_error));
}

int run_vecho(const char *pier_root, const char *message) {
    char exe_path[MAX_PATH_LEN];
    char cmdline[MAX_CMD_LEN];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    snprintf(exe_path, sizeof(exe_path), "%s\\bin\\vecho.exe", pier_root);
    snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", exe_path, message);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    memset(&pi, 0, sizeof(pi));

    if (!CreateProcessA(exe_path, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return 0;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 1;
}

/* Main function */
int main(int argc, char *argv[]) {
    char *pier_root = NULL;
    char *lang_dir = NULL;
    char *source = NULL;
    char *full_source_url = NULL;
    char *package = NULL;
    char *alias = NULL;
    int user_arg_start = 0;
    
    char alias_file[MAX_PATH_LEN];
    char metadata_file[MAX_PATH_LEN];
    char cache_dir[MAX_PATH_LEN];
    char installdir[MAX_PATH_LEN];
    char program_path[MAX_PATH_LEN];
    char alias_template[MAX_LINE];
    char alias_args[MAX_LINE];  /* Arguments part after program name */
    char actual_package[MAX_LINE];
    char temp_path[MAX_PATH_LEN];
    
    int is_third_party = 0;
    int need_cleanup = 0;
    int result = 0;
    
    /* Check minimum arguments */
    if (argc < 6) {
        printf("error: insufficient arguments\n");
        printf("usage: pier-op.exe <PIER_ROOT> <LANGUAGE_DIR> <source> <full_source_url> <package> [alias] [args...]\n");
        return 1;
    }
    
    /* Parse arguments */
    const char *alias_source = ALIAS_SOURCE;  /* Use hardcoded alias source */
    if (!parse_arguments(argc, argv, &pier_root, &lang_dir, &source, &full_source_url, &package, &alias, &user_arg_start)) {
        printf("error: failed to parse arguments\n");
        return 1;
    }
    
    /* Setup paths */
    char alias_cache_dir[MAX_PATH_LEN];
    snprintf(g_lang_file, sizeof(g_lang_file), "%s\\lang.ini", lang_dir);
    snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", pier_root);
    snprintf(alias_cache_dir, sizeof(alias_cache_dir), "%s\\metadata\\alias", pier_root);
    
    /* Initialize language strings */
    init_lang_strings();
    
    /* Check if it's third-party alias format (user/package) */
    is_third_party = is_third_party_alias(package);
    
    if (is_third_party) {
        /* Third-party alias: check local first, then download if needed */
        char user[MAX_LINE];
        char pkg[MAX_LINE];
        char *slash;
        int alias_exists_local = 0;
        
        /* Parse user/package to build local path */
        strncpy(user, package, sizeof(user) - 1);
        user[sizeof(user) - 1] = '\0';
        slash = strchr(user, '/');
        if (slash) {
            *slash = '\0';
            strncpy(pkg, slash + 1, sizeof(pkg) - 1);
            pkg[sizeof(pkg) - 1] = '\0';
        } else {
            printf("%s\n", g_open_alias_parse_error);
            return 1;
        }
        
        /* Check if alias file exists locally first */
        snprintf(alias_file, sizeof(alias_file), "%s\\%s\\%s.sque", alias_cache_dir, user, pkg);
        if (GetFileAttributesA(alias_file) != INVALID_FILE_ATTRIBUTES) {
            /* Local alias file exists, use it directly */
            alias_exists_local = 1;
        }
        
        /* If not found locally, try to download */
        if (!alias_exists_local) {
            printf("%s\n", g_loading_metadata);
            
            /* Create metadata directory first, then alias subdirectory */
            snprintf(temp_path, sizeof(temp_path), "%s\\metadata", pier_root);
            CreateDirectoryA(temp_path, NULL);
            snprintf(temp_path, sizeof(temp_path), "%s\\metadata\\alias", pier_root);
            CreateDirectoryA(temp_path, NULL);
            
            /* Download alias template using alias_source */
            if (!download_alias_template(lang_dir, alias_source, package, alias_cache_dir, alias_file, sizeof(alias_file))) {
                run_vecho(pier_root, g_open_alias_not_found_template);
                return 1;
            }
        }
        
        /* Parse [ToUse] to get actual package name */
        if (!parse_alias_touse(alias_file, actual_package, sizeof(actual_package))) {
            printf("%s\n", g_open_alias_parse_error);
            return 1;
        }
        
        need_cleanup = 1;
    } else {
        /* Regular package */
        strncpy(actual_package, package, sizeof(actual_package) - 1);
        actual_package[sizeof(actual_package) - 1] = '\0';
        alias_file[0] = '\0'; /* No alias file */
    }
    
    /* Get metadata file path */
    snprintf(metadata_file, sizeof(metadata_file), "%s\\metadata\\%s.sque", pier_root, actual_package);
    
    /* If metadata doesn't exist, try to download it */
    if (GetFileAttributesA(metadata_file) == INVALID_FILE_ATTRIBUTES) {
        printf("%s\n", g_loading_metadata);
        
        /* Clean and create cache directory */
        snprintf(temp_path, sizeof(temp_path), "%s\\*.*", cache_dir);
        
        /* Download metadata */
        if (!download_metadata(lang_dir, full_source_url, actual_package, cache_dir)) {
            char vecho_cmd[MAX_CMD_LEN];
            printf("%s\n", g_open_metadata_error);
            /* Use vecho to highlight the install command */
            snprintf(vecho_cmd, sizeof(vecho_cmd), "%s $brightyellow$pier install %s$write$", g_open_alias_install_hint, actual_package);
            run_vecho(pier_root, vecho_cmd);
            return 1;
        }
        
        /* Use downloaded metadata */
        snprintf(metadata_file, sizeof(metadata_file), "%s\\metadata.sque", cache_dir);
        
        /* Save metadata with localized Chinese content merged from profile.sque and notice.sque */
        {
            char metadata_dst[MAX_PATH_LEN];
            char profile_path[MAX_PATH_LEN], notice_path[MAX_PATH_LEN];
            char profile_buf[MAX_LINE], notice_buf[MAX_LINE];
            char line[MAX_LINE];
            char *profile_pkgname = NULL, *profile_desc = NULL;
            char *lang_code;
            FILE *fin, *fout;
            int skip_section = 0;
            
            lang_code = strrchr(lang_dir, '\\');
            if (lang_code) lang_code++; else lang_code = "zh-CN";
            
            snprintf(profile_path, sizeof(profile_path), "%s\\profile.sque", cache_dir);
            if (sque_read(profile_path, lang_code, profile_buf, sizeof(profile_buf)) >= 0) {
                char *line_ptr = strtok(profile_buf, "\n");
                while (line_ptr) {
                    if (strncmp(line_ptr, "PackageName:", 12) == 0) {
                        profile_pkgname = line_ptr + 12;
                        while (*profile_pkgname == ' ' || *profile_pkgname == '\t') profile_pkgname++;
                    } else if (strncmp(line_ptr, "ProFile:", 8) == 0) {
                        profile_desc = line_ptr + 8;
                        while (*profile_desc == ' ' || *profile_desc == '\t') profile_desc++;
                    }
                    line_ptr = strtok(NULL, "\n");
                }
            }
            
            snprintf(notice_path, sizeof(notice_path), "%s\\notice.sque", cache_dir);
            if (sque_read(notice_path, lang_code, notice_buf, sizeof(notice_buf)) < 0) {
                notice_buf[0] = '\0';
            }
            
            snprintf(metadata_dst, sizeof(metadata_dst), "%s\\metadata", pier_root);
            CreateDirectoryA(metadata_dst, NULL);
            snprintf(metadata_dst, sizeof(metadata_dst), "%s\\metadata\\%s.sque", pier_root, actual_package);
            
            fin = fopen(metadata_file, "r");
            fout = fopen(metadata_dst, "w");
            if (fin && fout) {
                while (fgets(line, sizeof(line), fin)) {
                    size_t len = strlen(line);
                    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
                    
                    if (strcmp(line, "[PackageName]") == 0) {
                        if (profile_pkgname && profile_pkgname[0]) {
                            fprintf(fout, "[PackageName]\n%s\n::end\n\n", profile_pkgname);
                            skip_section = 1;
                        } else {
                            fprintf(fout, "[PackageName]\n");
                        }
                        continue;
                    }
                    if (strcmp(line, "[ProFile]") == 0) {
                        if (profile_desc && profile_desc[0]) {
                            fprintf(fout, "[ProFile]\n%s\n::end\n\n", profile_desc);
                            skip_section = 1;
                        } else {
                            fprintf(fout, "[ProFile]\n");
                        }
                        continue;
                    }
                    if (strcmp(line, "[Notice]") == 0) {
                        if (notice_buf[0]) {
                            fprintf(fout, "[Notice]\n%s\n::end\n\n", notice_buf);
                            skip_section = 1;
                        } else {
                            fprintf(fout, "[Notice]\n");
                        }
                        continue;
                    }
                    
                    if (skip_section) {
                        if (strcmp(line, "::end") == 0) {
                            skip_section = 0;
                            continue;
                        }
                        if (line[0] == '[') {
                            skip_section = 0;
                        } else {
                            continue;
                        }
                    }
                    
                    fprintf(fout, "%s\n", line);
                }
                fclose(fout);
            }
            if (fin) fclose(fin);
        }
        
        /* Use saved merged metadata instead of cache for Chinese display */
        snprintf(metadata_file, sizeof(metadata_file), "%s\\metadata\\%s.sque", pier_root, actual_package);
        
        need_cleanup = 1;
        
        /* Verify package is installed */
        {
            char check_installer[MAX_LINE];
            char check_path[MAX_PATH_LEN];
            
            if (sque_read(metadata_file, "InstallerName", check_installer, sizeof(check_installer)) < 0) {
                printf("%s\n", g_open_metadata_error);
                return 1;
            }
            
            snprintf(check_path, sizeof(check_path), "%s\\app\\%s", pier_root, check_installer);
            if (GetFileAttributesA(check_path) == INVALID_FILE_ATTRIBUTES) {
                char vecho_cmd[MAX_CMD_LEN];
                printf("%s %s\n", g_open_package_not_found, actual_package);
                /* Use vecho to highlight the install command */
                snprintf(vecho_cmd, sizeof(vecho_cmd), "%s $brightyellow$pier install %s$write$", g_open_alias_install_hint, actual_package);
                run_vecho(pier_root, vecho_cmd);
                return 1;
            }
            
            /* Check if directory is empty (folder exists but content deleted) */
            {
                WIN32_FIND_DATAA findData;
                HANDLE hFind;
                char search_path[MAX_PATH_LEN];
                int file_count = 0;
                
                snprintf(search_path, sizeof(search_path), "%s\\*", check_path);
                hFind = FindFirstFileA(search_path, &findData);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        /* Skip . and .. */
                        if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                            file_count++;
                        }
                    } while (FindNextFileA(hFind, &findData));
                    FindClose(hFind);
                }
                
                if (file_count == 0) {
                    /* Directory is empty - treat as not installed */
                    char vecho_cmd[MAX_CMD_LEN];
                    printf("%s %s\n", g_open_package_not_found, actual_package);
                    /* Use vecho to highlight the install command */
                    snprintf(vecho_cmd, sizeof(vecho_cmd), "%s $brightyellow$pier install %s$write$", g_open_alias_install_hint, actual_package);
                    run_vecho(pier_root, vecho_cmd);
                    return 1;
                }
            }
        }
    }
    
    /* Read InstallerName from metadata */
    if (sque_read(metadata_file, "InstallerName", installdir, sizeof(installdir)) < 0) {
        printf("%s\n", g_open_metadata_error);
        return 1;
    }
    
    /* Determine program to run */
    if (alias != NULL && strlen(alias) > 0) {
        int alias_found = 0;
        /* Try to find alias */
        if (strlen(alias_file) > 0) {
            /* Try third-party alias file first */
            if (extract_alias_section(alias_file, alias, alias_template, sizeof(alias_template))) {
                alias_found = 1;
            } else if (extract_alias_section(metadata_file, alias, alias_template, sizeof(alias_template))) {
                alias_found = 1;
            }
        } else {
            /* Try metadata alias only */
            if (extract_alias_section(metadata_file, alias, alias_template, sizeof(alias_template))) {
                alias_found = 1;
            }
        }
        
        if (alias_found) {
            /* Build program path from alias template */
            char *space_pos;
            trim_whitespace(alias_template);
            
            /* Extract program name (first word before space) */
            space_pos = strchr(alias_template, ' ');
            if (space_pos) {
                /* Save arguments part */
                strncpy(alias_args, space_pos + 1, sizeof(alias_args) - 1);
                alias_args[sizeof(alias_args) - 1] = '\0';
                *space_pos = '\0';  /* Terminate program name */
                snprintf(program_path, sizeof(program_path), "%s\\app\\%s\\%s", pier_root, installdir, alias_template);
            } else {
                /* No space, entire template is program name */
                alias_args[0] = '\0';
                snprintf(program_path, sizeof(program_path), "%s\\app\\%s\\%s", pier_root, installdir, alias_template);
            }
        } else {
            /* Alias not found, treat as parameter for default program */
            /* Show available aliases */
            printf("%s %s\n", g_error_alias_not_found, alias);
            printf("\n%s\n", g_open_available_aliases);
            {
                char content[MAX_LINE * 8];
                if (sque_read(metadata_file, "Alias", content, sizeof(content)) >= 0) {
                    char *line_ptr = strtok(content, "\n");
                    while (line_ptr) {
                        if (strlen(line_ptr) > 0 && strchr(line_ptr, ':')) {
                            char *colon = strchr(line_ptr, ':');
                            *colon = '\0';
                            printf("  %-10s - %s\n", line_ptr, colon + 1);
                        }
                        line_ptr = strtok(NULL, "\n");
                    }
                }
            }
            printf("\n");
            /* Use DefaultOpen and pass alias and all subsequent args */
            if (!extract_defaultopen(metadata_file, installdir, pier_root, program_path, sizeof(program_path))) {
                printf("%s\n", g_error_no_default_open);
                return 1;
            }
            /* Build args string from alias (argv[6]) onwards */
            int j;
            alias_args[0] = '\0';
            for (j = 6; j < argc; j++) {
                strncat(alias_args, " \"", sizeof(alias_args) - strlen(alias_args) - 1);
                strncat(alias_args, argv[j], sizeof(alias_args) - strlen(alias_args) - 1);
                strncat(alias_args, "\"", sizeof(alias_args) - strlen(alias_args) - 1);
            }
            user_arg_start = argc; /* All args already embedded in alias_args */
        }
    } else {
        /* Use DefaultOpen */
        if (!extract_defaultopen(metadata_file, installdir, pier_root, program_path, sizeof(program_path))) {
            printf("%s\n", g_error_no_default_open);
            return 1;
        }
        alias_args[0] = '\0'; /* No template for default open */
    }
    
    /* Verify program exists */
    if (GetFileAttributesA(program_path) == INVALID_FILE_ATTRIBUTES) {
        /* Try with .exe extension */
        strncat(program_path, ".exe", sizeof(program_path) - strlen(program_path) - 1);
        if (GetFileAttributesA(program_path) == INVALID_FILE_ATTRIBUTES) {
            printf("%s %s\n", g_open_file_not_found, program_path);
            return 1;
        }
    }
    
    /* Get package name for display */
    {
        char pkg_name[MAX_LINE];
        if (sque_read(metadata_file, "PackageName", pkg_name, sizeof(pkg_name)) >= 0) {
            printf("%s %s\n", g_open_program, pkg_name);
        } else {
            printf("%s %s\n", g_open_program, actual_package);
        }
        printf("\n");
    }
    
    /* Build and execute command with all user arguments */
    build_and_execute(program_path, alias_args, argc, argv, user_arg_start);
    
    /* Cleanup if needed */
    if (need_cleanup) {
        /* Note: We don't delete cache files immediately to allow reuse */
    }
    
    return 0;
}

/* Parse command line arguments */
int parse_arguments(int argc, char *argv[], char **pier_root, char **lang_dir, char **source, char **full_source_url, char **package, char **alias, int *user_arg_start) {
    *pier_root = argv[1];
    *lang_dir = argv[2];
    *source = argv[3];
    *full_source_url = argv[4];
    *package = argv[5];
    
    /* Check for alias (argv[6]) */
    if (argc > 6) {
        /* Check if argv[6] starts with - or / (option) */
        if (argv[6][0] != '-' && argv[6][0] != '/') {
            *alias = argv[6];
            *user_arg_start = 7;
        } else {
            *alias = NULL;
            *user_arg_start = 6;
        }
    } else {
        *alias = NULL;
        *user_arg_start = 6;
    }
    
    return 1;
}

/* Check if package is in third-party format (user/package) */
int is_third_party_alias(const char *package) {
    /* Check for / in package name (but not at start or end) */
    const char *slash = strchr(package, '/');
    if (slash != NULL && slash != package && *(slash + 1) != '\0') {
        return 1;
    }
    return 0;
}

/* Download alias template from source */
int download_alias_template(const char *lang_dir, const char *source, const char *package, const char *cache_dir, char *alias_file, int alias_file_size) {
    char url[MAX_PATH_LEN];
    char cmd[MAX_CMD_LEN];
    char alias_dir[MAX_PATH_LEN];
    char *slash;
    char user[MAX_LINE];
    char pkg[MAX_LINE];
    
    /* Parse user/package */
    strncpy(user, package, sizeof(user) - 1);
    user[sizeof(user) - 1] = '\0';
    slash = strchr(user, '/');
    if (slash) {
        *slash = '\0';
        strncpy(pkg, slash + 1, sizeof(pkg) - 1);
        pkg[sizeof(pkg) - 1] = '\0';
    } else {
        return 0;
    }
    
    /* Get first letter of username for categorization */
    char first_letter[2] = {0};
    first_letter[0] = tolower(user[0]);
    
    /* Create user directory (cache_dir is already metadata/alias) */
    snprintf(alias_dir, sizeof(alias_dir), "%s\\%s", cache_dir, user);
    CreateDirectoryA(alias_dir, NULL);
    
    /* Build URL: source/alias/<first_letter>/user/package.sque */
    snprintf(url, sizeof(url), "%s/alias/%s/%s/%s.sque", source, first_letter, user, pkg);
    
    /* Build output path */
    snprintf(alias_file, alias_file_size, "%s\\%s.sque", alias_dir, pkg);
    
    /* OLD (pier-get): snprintf(pg_cmd, sizeof(pg_cmd), "pier-get.exe \"%s\" \"%s\" \"%s\"", lang_dir, url, alias_file); */
    /* Download using uma-get */
    {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        char pg_cmd[MAX_CMD_LEN];
        
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        
        {
            char proxy_opts[512];
            build_wget_proxy_opts(proxy_opts, sizeof(proxy_opts));
            snprintf(pg_cmd, sizeof(pg_cmd), "uma-get.exe -q --timeout=30 --tries=2 --no-check-certificate%s -O \"%s\" \"%s\"", proxy_opts, alias_file, url);
        }
        if (!CreateProcessA(NULL, pg_cmd, NULL, NULL, FALSE,
                           CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            /* CREATE_NO_WINDOW requires Vista+, fallback for XP */
            if (!CreateProcessA(NULL, pg_cmd, NULL, NULL, FALSE,
                               0, NULL, NULL, &si, &pi)) {
                return 0;
            }
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (GetFileAttributesA(alias_file) == INVALID_FILE_ATTRIBUTES) {
            return 0;
        }
    }
    
    return 1;
}

/* Download metadata for package */
int download_metadata(const char *lang_dir, const char *full_source_url, const char *package_name, const char *cache_dir) {
    char url[MAX_PATH_LEN];
    char cmd[MAX_CMD_LEN];
    char temp_file[MAX_PATH_LEN];
    char metadata_file[MAX_PATH_LEN];
    char installer_buf[MAX_LINE];
    
    /* Build URL */
    snprintf(url, sizeof(url), "%s/%c/%s/latest.metadata", full_source_url, package_name[0], package_name);
    snprintf(temp_file, sizeof(temp_file), "%s\\%s.metadata", getenv("TEMP") ? getenv("TEMP") : "C:\\Windows\\Temp", package_name);
    snprintf(metadata_file, sizeof(metadata_file), "%s\\metadata.sque", cache_dir);
    
    /* Clean old cache files to prevent pollution from previous downloads */
    {
        char sque_path[MAX_PATH_LEN];
        snprintf(sque_path, sizeof(sque_path), "%s\\metadata.sque", cache_dir);
        remove(sque_path);
        snprintf(sque_path, sizeof(sque_path), "%s\\notice.sque", cache_dir);
        remove(sque_path);
        snprintf(sque_path, sizeof(sque_path), "%s\\profile.sque", cache_dir);
        remove(sque_path);
    }
    
    /* OLD (pier-get): snprintf(pg_cmd, sizeof(pg_cmd), "pier-get.exe -q \"%s\" \"%s\" \"%s\"", lang_dir, url, temp_file); */
    /* Download using uma-get */
    {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        char pg_cmd[MAX_CMD_LEN];
        
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        
        {
            char proxy_opts[512];
            build_wget_proxy_opts(proxy_opts, sizeof(proxy_opts));
            snprintf(pg_cmd, sizeof(pg_cmd), "uma-get.exe -q --timeout=30 --tries=2 --no-check-certificate%s -O \"%s\" \"%s\"", proxy_opts, temp_file, url);
        }
        if (!CreateProcessA(NULL, pg_cmd, NULL, NULL, FALSE,
                           CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            /* CREATE_NO_WINDOW requires Vista+, fallback for XP */
            if (!CreateProcessA(NULL, pg_cmd, NULL, NULL, FALSE,
                               0, NULL, NULL, &si, &pi)) {
                return 0;
            }
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (GetFileAttributesA(temp_file) == INVALID_FILE_ATTRIBUTES) {
            return 0;
        }
    }
    
    /* Unzip metadata */
    {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        char unzip_cmd[MAX_CMD_LEN];
        
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        
        snprintf(unzip_cmd, sizeof(unzip_cmd), "unzip.exe \"%s\" -d \"%s\"", temp_file, cache_dir);
        if (CreateProcessA(NULL, unzip_cmd, NULL, NULL, FALSE,
                          CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        } else {
            /* CREATE_NO_WINDOW requires Vista+, fallback for XP */
            if (CreateProcessA(NULL, unzip_cmd, NULL, NULL, FALSE,
                              0, NULL, NULL, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
    }
    
    /* Delete temp file */
    DeleteFileA(temp_file);
    
    /* Check if metadata.sque exists AND has valid InstallerName */
    if (GetFileAttributesA(metadata_file) == INVALID_FILE_ATTRIBUTES) {
        return 0;
    }
    
    if (sque_read(metadata_file, "InstallerName", installer_buf, sizeof(installer_buf)) < 0 ||
        strlen(installer_buf) == 0) {
        return 0;
    }
    
    return 1;
}

/* Parse [ToUse] field from alias template */
int parse_alias_touse(const char *alias_file, char *package_name, int package_name_size) {
    return sque_read(alias_file, "ToUse", package_name, package_name_size) >= 0;
}

/* Extract alias command from [Alias] section using sque library */
int extract_alias_section(const char *filepath, const char *alias_name, char *program, int program_size) {
    char content[MAX_LINE * 8];
    char *line_ptr;
    
    if (sque_read(filepath, "Alias", content, sizeof(content)) < 0) {
        return 0;
    }
    
    line_ptr = strtok(content, "\n");
    while (line_ptr) {
        if (strlen(line_ptr) > 0 && strchr(line_ptr, ':')) {
            char *colon = strchr(line_ptr, ':');
            char current_alias[MAX_LINE];
            int alias_len = (int)(colon - line_ptr);
            
            if (alias_len >= (int)sizeof(current_alias))
                alias_len = (int)sizeof(current_alias) - 1;
            memcpy(current_alias, line_ptr, alias_len);
            current_alias[alias_len] = '\0';
            trim_whitespace(current_alias);
            
            if (strcmpi(current_alias, alias_name) == 0) {
                strncpy(program, colon + 1, program_size - 1);
                program[program_size - 1] = '\0';
                trim_whitespace(program);
                return 1;
            }
        }
        line_ptr = strtok(NULL, "\n");
    }
    
    return 0;
}

/* Extract program from [DefaultOpen] section using sque library */
int extract_defaultopen(const char *metadata_file, const char *installdir, const char *pier_root, char *program_path, int program_path_size) {
    char content[MAX_LINE * 8];
    char *line_ptr;
    
    if (sque_read(metadata_file, "DefaultOpen", content, sizeof(content)) < 0) {
        return 0;
    }
    
    line_ptr = strtok(content, "\n");
    while (line_ptr) {
        if (strlen(line_ptr) > 0) {
            char test_path[MAX_PATH_LEN];
            int has_wildcard = (strchr(line_ptr, '*') || strchr(line_ptr, '?'));
            
            if (has_wildcard) {
                WIN32_FIND_DATAA findData;
                HANDLE hFind;
                char search_path[MAX_PATH_LEN];
                
                snprintf(search_path, sizeof(search_path), "%s\\app\\%s\\%s", pier_root, installdir, line_ptr);
                hFind = FindFirstFileA(search_path, &findData);
                if (hFind != INVALID_HANDLE_VALUE) {
                    snprintf(program_path, program_path_size, "%s\\app\\%s\\%s", pier_root, installdir, findData.cFileName);
                    FindClose(hFind);
                    return 1;
                }
            } else {
                snprintf(test_path, sizeof(test_path), "%s\\app\\%s\\%s", pier_root, installdir, line_ptr);
                if (GetFileAttributesA(test_path) != INVALID_FILE_ATTRIBUTES) {
                    strncpy(program_path, test_path, program_path_size - 1);
                    program_path[program_path_size - 1] = '\0';
                    return 1;
                }
                strncat(test_path, ".exe", sizeof(test_path) - strlen(test_path) - 1);
                if (GetFileAttributesA(test_path) != INVALID_FILE_ATTRIBUTES) {
                    strncpy(program_path, test_path, program_path_size - 1);
                    program_path[program_path_size - 1] = '\0';
                    return 1;
                }
            }
        }
        line_ptr = strtok(NULL, "\n");
    }
    
    return 0;
}

/* Build and execute command with parameter substitution */
void build_and_execute(const char *program_path, const char *alias_template, int argc, char *argv[], int user_arg_start) {
    char cmd[MAX_CMD_LEN];
    char final_cmd[MAX_CMD_LEN];
    char *user_args[MAX_ARGS];
    int user_arg_count = 0;
    int used_args[MAX_ARGS];
    int i;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    
    /* Collect user arguments */
    for (i = user_arg_start; i < argc && user_arg_count < MAX_ARGS; i++) {
        user_args[user_arg_count] = argv[i];
        used_args[user_arg_count] = 0;
        user_arg_count++;
    }
    
    /* Initialize command */
    final_cmd[0] = '\0';
    
    if (alias_template != NULL && strlen(alias_template) > 0) {
        /* Replace $1, $2, etc. in template */
        replace_placeholders(alias_template, user_args, user_arg_count, used_args, final_cmd, sizeof(final_cmd));
    }
    
    /* Append unused arguments */
    for (i = 0; i < user_arg_count; i++) {
        if (!used_args[i]) {
            strncat(final_cmd, " ", sizeof(final_cmd) - strlen(final_cmd) - 1);
            strncat(final_cmd, user_args[i], sizeof(final_cmd) - strlen(final_cmd) - 1);
        }
    }
    
    /* Build full command */
    snprintf(cmd, sizeof(cmd), "\"%s\" %s", program_path, final_cmd);
    
    /* Change to PIER_ROOT directory */
    {
        char pier_root[MAX_PATH_LEN];
        strncpy(pier_root, program_path, sizeof(pier_root) - 1);
        pier_root[sizeof(pier_root) - 1] = '\0';
        
        /* Extract PIER_ROOT from program_path (remove \app\...) */
        {
            char *app_pos = strstr(pier_root, "\\app\\");
            if (app_pos) {
                *app_pos = '\0';
            }
        }
        
        SetCurrentDirectoryA(pier_root);
    }
    
    /* Execute using CreateProcess for better control */
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        printf("error: failed to start program (error: %lu)\n", GetLastError());
    }
}

/* Replace $1, $2, etc. placeholders with actual arguments */
void replace_placeholders(const char *template, char **user_args, int user_arg_count, int *used_args, char *output, int output_size) {
    const char *p = template;
    char *out = output;
    int out_len = 0;
    
    output[0] = '\0';
    
    while (*p && out_len < output_size - 1) {
        if (*p == '$' && *(p + 1) >= '0' && *(p + 1) <= '9') {
            /* Parse multi-digit number */
            int arg_num = 0;
            const char *num_start = p + 1;
            while (*num_start >= '0' && *num_start <= '9') {
                arg_num = arg_num * 10 + (*num_start - '0');
                num_start++;
            }
            /* $0 is not valid, must be $1 or higher */
            if (arg_num >= 1 && arg_num <= user_arg_count) {
                int arg_len = strlen(user_args[arg_num - 1]);
                if (out_len + arg_len < output_size - 1) {
                    strcpy(out, user_args[arg_num - 1]);
                    out += arg_len;
                    out_len += arg_len;
                    used_args[arg_num - 1] = 1;
                }
            }
            p = num_start;
        } else {
            *out++ = *p++;
            out_len++;
        }
    }
    *out = '\0';
}

/* Trim whitespace from string */
void trim_whitespace(char *str) {
    char *start = str;
    char *end;
    
    while (*start == ' ' || *start == '\t') {
        start++;
    }
    
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
}

/* Extract filename from path */
void extract_filename(char *str) {
    char *last_slash = strrchr(str, '/');
    char *last_backslash = strrchr(str, '\\');
    
    if (last_slash && last_backslash) {
        if (last_slash > last_backslash) {
            memmove(str, last_slash + 1, strlen(last_slash + 1) + 1);
        } else {
            memmove(str, last_backslash + 1, strlen(last_backslash + 1) + 1);
        }
    } else if (last_slash) {
        memmove(str, last_slash + 1, strlen(last_slash + 1) + 1);
    } else if (last_backslash) {
        memmove(str, last_backslash + 1, strlen(last_backslash + 1) + 1);
    }
}

/* Check if string starts with prefix */
int starts_with(const char *str, const char *prefix) {
    size_t prefix_len = strlen(prefix);
    return (strlen(str) >= prefix_len && strncmp(str, prefix, prefix_len) == 0);
}

/* Case-insensitive string compare */
int strcmpi(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        
        if (c1 != c2) {
            return c1 - c2;
        }
        
        s1++;
        s2++;
    }
    
    return *s1 - *s2;
}

/* URL encode string (simplified for XP compatibility) */
void url_encode(const char *input, char *output, int output_size) {
    const char *p = input;
    char *out = output;
    int out_len = 0;
    
    while (*p && out_len < output_size - 4) {
        if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
            (*p >= '0' && *p <= '9') || *p == '-' || *p == '_' ||
            *p == '.' || *p == '~') {
            *out++ = *p++;
            out_len++;
        } else if (*p == ' ') {
            *out++ = '%';
            *out++ = '2';
            *out++ = '0';
            p++;
            out_len += 3;
        } else {
            sprintf(out, "%%%02X", (unsigned char)*p);
            out += 3;
            p++;
            out_len += 3;
        }
    }
    *out = '\0';
}
