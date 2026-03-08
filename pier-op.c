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
 * Return codes:
 *   0 - Success
 *   1 - Error (metadata not found, file not found, etc.)
 *   2 - Package not installed (for third-party alias)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_LINE 1024
#define MAX_PATH_LEN 512
#define MAX_ARGS 256
#define MAX_CMD_LEN 8192

/* Function prototypes */
int parse_arguments(int argc, char *argv[], char **pier_root, char **lang_dir, char **source, char **full_source_url, char **alias_source, char **package, char **alias, int *user_arg_start);
int is_third_party_alias(const char *package);
int download_alias_template(const char *source, const char *package, const char *cache_dir, char *alias_file, int alias_file_size);
int download_metadata(const char *full_source_url, const char *package_name, const char *cache_dir);
int parse_alias_touse(const char *alias_file, char *package_name, int package_name_size);
int read_metadata_field(const char *filepath, const char *fieldname, char *output, int output_size);
int extract_alias_section(const char *alias_file, const char *alias_name, char *program, int program_size);
int extract_defaultopen(const char *metadata_file, const char *installdir, const char *pier_root, char *program_path, int program_path_size);
void build_and_execute(const char *program_path, const char *alias_template, int argc, char *argv[], int user_arg_start);
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

/* Read language string from lang.ini */
void read_lang_string(const char *fieldname, char *output, int output_size) {
    FILE *fp;
    char line[MAX_LINE];
    char field_tag[MAX_LINE];
    int in_field = 0;
    
    fp = fopen(g_lang_file, "r");
    if (fp == NULL) {
        output[0] = '\0';
        return;
    }
    
    snprintf(field_tag, sizeof(field_tag), "[%s]", fieldname);
    output[0] = '\0';
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        newline = strchr(line, '\r');
        if (newline) *newline = '\0';
        
        if (in_field) {
            if (line[0] == '[' && strchr(line, ']') != NULL) {
                break;
            }
            if (strlen(output) == 0 && strlen(line) > 0) {
                strncpy(output, line, output_size - 1);
                output[output_size - 1] = '\0';
                break;
            }
        } else if (strcmpi(line, field_tag) == 0) {
            in_field = 1;
        }
    }
    
    fclose(fp);
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
    char actual_package[MAX_LINE];
    char temp_path[MAX_PATH_LEN];
    
    int is_third_party = 0;
    int need_cleanup = 0;
    int result = 0;
    
    /* Check minimum arguments */
    if (argc < 7) {
        printf("error: insufficient arguments\n");
        printf("usage: pier-op.exe <PIER_ROOT> <LANGUAGE_DIR> <source> <full_source_url> <alias_source> <package> [alias] [args...]\n");
        return 1;
    }
    
    /* Parse arguments */
    char *alias_source = NULL;
    if (!parse_arguments(argc, argv, &pier_root, &lang_dir, &source, &full_source_url, &alias_source, &package, &alias, &user_arg_start)) {
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
        /* Third-party alias: download alias template first */
        printf("%s\n", g_loading_metadata);
        
        /* Create metadata directory first, then alias subdirectory */
        snprintf(temp_path, sizeof(temp_path), "%s\\metadata", pier_root);
        CreateDirectoryA(temp_path, NULL);
        snprintf(temp_path, sizeof(temp_path), "%s\\metadata\\alias", pier_root);
        CreateDirectoryA(temp_path, NULL);
        
        /* Download alias template using alias_source */
        if (!download_alias_template(alias_source, package, alias_cache_dir, alias_file, sizeof(alias_file))) {
            char vecho_cmd[MAX_CMD_LEN];
            snprintf(vecho_cmd, sizeof(vecho_cmd), "%s\\bin\\vecho.exe %s", pier_root, g_open_alias_not_found_template);
            system(vecho_cmd);
            return 1;
        }
        
        /* Parse [ToUse] to get actual package name */
        if (!parse_alias_touse(alias_file, actual_package, sizeof(actual_package))) {
            printf("%s\n", g_open_alias_parse_error);
            return 1;
        }
        
        /* Check if actual package is installed */
        snprintf(metadata_file, sizeof(metadata_file), "%s\\metadata\\%s.sque", pier_root, actual_package);
        if (GetFileAttributesA(metadata_file) == INVALID_FILE_ATTRIBUTES) {
            /* Package not installed - prompt user */
            char vecho_cmd[MAX_CMD_LEN];
            printf("%s %s\n", g_open_alias_not_installed, actual_package);
            fflush(stdout);
            /* Use vecho to highlight the install command */
            snprintf(vecho_cmd, sizeof(vecho_cmd), "%s\\bin\\vecho.exe %s $brightyellow$pier install %s$write$", pier_root, g_open_alias_install_hint, actual_package);
            system(vecho_cmd);
            return 2;
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
        if (!download_metadata(full_source_url, actual_package, cache_dir)) {
            char vecho_cmd[MAX_CMD_LEN];
            printf("%s\n", g_open_metadata_error);
            /* Use vecho to highlight the install command */
            snprintf(vecho_cmd, sizeof(vecho_cmd), "%s\\bin\\vecho.exe %s $brightyellow$pier install %s$write$", pier_root, g_open_alias_install_hint, actual_package);
            system(vecho_cmd);
            return 1;
        }
        
        /* Use downloaded metadata */
        snprintf(metadata_file, sizeof(metadata_file), "%s\\metadata.sque", cache_dir);
        need_cleanup = 1;
        
        /* Verify package is installed */
        {
            char check_installdir[MAX_LINE];
            char check_path[MAX_PATH_LEN];
            
            if (!read_metadata_field(metadata_file, "InstallDir", check_installdir, sizeof(check_installdir))) {
                printf("%s\n", g_open_metadata_error);
                return 1;
            }
            
            /* Remove leading backslash for XP compatibility */
            if (check_installdir[0] == '\\') {
                memmove(check_installdir, check_installdir + 1, strlen(check_installdir));
            }
            
            snprintf(check_path, sizeof(check_path), "%s\\app\\%s", pier_root, check_installdir);
            if (GetFileAttributesA(check_path) == INVALID_FILE_ATTRIBUTES) {
                char vecho_cmd[MAX_CMD_LEN];
                printf("%s %s\n", g_open_package_not_found, actual_package);
                /* Use vecho to highlight the install command */
                snprintf(vecho_cmd, sizeof(vecho_cmd), "%s\\bin\\vecho.exe %s $brightyellow$pier install %s$write$", pier_root, g_open_alias_install_hint, actual_package);
                system(vecho_cmd);
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
                    snprintf(vecho_cmd, sizeof(vecho_cmd), "%s\\bin\\vecho.exe %s $brightyellow$pier install %s$write$", pier_root, g_open_alias_install_hint, actual_package);
                    system(vecho_cmd);
                    return 1;
                }
            }
        }
    }
    
    /* Read InstallDir from metadata */
    if (!read_metadata_field(metadata_file, "InstallDir", installdir, sizeof(installdir))) {
        printf("%s\n", g_open_metadata_error);
        return 1;
    }
    
    /* Remove leading backslash for XP compatibility */
    if (installdir[0] == '\\') {
        memmove(installdir, installdir + 1, strlen(installdir));
    }
    
    /* Determine program to run */
    if (alias != NULL && strlen(alias) > 0) {
        /* Try to find alias */
        if (strlen(alias_file) > 0) {
            /* Try third-party alias file first */
            if (!extract_alias_section(alias_file, alias, alias_template, sizeof(alias_template))) {
                /* Try metadata alias */
                if (!extract_alias_section(metadata_file, alias, alias_template, sizeof(alias_template))) {
                    printf("%s %s\n", g_error_alias_not_found, alias);
                    printf("\n%s\n", g_open_available_aliases);
                    /* List available aliases */
                    {
                        FILE *fp = fopen(metadata_file, "r");
                        if (fp) {
                            char line[MAX_LINE];
                            int in_alias = 0;
                            while (fgets(line, sizeof(line), fp)) {
                                char *newline = strchr(line, '\n');
                                if (newline) *newline = '\0';
                                newline = strchr(line, '\r');
                                if (newline) *newline = '\0';
                                
                                if (in_alias) {
                                    if (line[0] == '[' || strcmp(line, "::end") == 0) break;
                                    if (strlen(line) > 0 && strchr(line, ':')) {
                                        char *colon = strchr(line, ':');
                                        *colon = '\0';
                                        printf("  %s\n", line);
                                    }
                                } else if (strcmpi(line, "[Alias]") == 0) {
                                    in_alias = 1;
                                }
                            }
                            fclose(fp);
                        }
                    }
                    return 1;
                }
            }
        } else {
            /* Try metadata alias only */
            if (!extract_alias_section(metadata_file, alias, alias_template, sizeof(alias_template))) {
                printf("%s %s\n", g_error_alias_not_found, alias);
                printf("\n%s\n", g_open_available_aliases);
                return 1;
            }
        }
        
        /* Build program path from alias template */
        trim_whitespace(alias_template);
        snprintf(program_path, sizeof(program_path), "%s\\app\\%s\\%s", pier_root, installdir, alias_template);
    } else {
        /* Use DefaultOpen */
        if (!extract_defaultopen(metadata_file, installdir, pier_root, program_path, sizeof(program_path))) {
            printf("%s\n", g_error_no_default_open);
            return 1;
        }
        alias_template[0] = '\0'; /* No template for default open */
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
        if (read_metadata_field(metadata_file, "PackageName", pkg_name, sizeof(pkg_name))) {
            printf("%s %s\n", g_open_program, pkg_name);
        } else {
            printf("%s %s\n", g_open_program, actual_package);
        }
        printf("\n");
    }
    
    /* Build and execute command with all user arguments */
    build_and_execute(program_path, alias_template, argc, argv, user_arg_start);
    
    /* Cleanup if needed */
    if (need_cleanup) {
        /* Note: We don't delete cache files immediately to allow reuse */
    }
    
    return 0;
}

/* Parse command line arguments */
int parse_arguments(int argc, char *argv[], char **pier_root, char **lang_dir, char **source, char **full_source_url, char **alias_source, char **package, char **alias, int *user_arg_start) {
    *pier_root = argv[1];
    *lang_dir = argv[2];
    *source = argv[3];
    *full_source_url = argv[4];
    *alias_source = argv[5];
    *package = argv[6];
    
    /* Check for alias (argv[7]) */
    if (argc > 7) {
        /* Check if argv[7] starts with - or / (option) */
        if (argv[7][0] != '-' && argv[7][0] != '/') {
            *alias = argv[7];
            *user_arg_start = 8;
        } else {
            *alias = NULL;
            *user_arg_start = 7;
        }
    } else {
        *alias = NULL;
        *user_arg_start = 7;
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
int download_alias_template(const char *source, const char *package, const char *cache_dir, char *alias_file, int alias_file_size) {
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
    
    /* Create user directory (cache_dir is already metadata/alias) */
    snprintf(alias_dir, sizeof(alias_dir), "%s\\%s", cache_dir, user);
    CreateDirectoryA(alias_dir, NULL);
    
    /* Build URL: source/alias/user/package.sque */
    snprintf(url, sizeof(url), "%s/alias/%s/%s.sque", source, user, pkg);
    
    /* Build output path */
    snprintf(alias_file, alias_file_size, "%s\\%s.sque", alias_dir, pkg);
    
    /* Download using curl or uma-get */
    /* Try curl first with -f flag to fail on HTTP errors (404, etc.), then uma-get */
    snprintf(cmd, sizeof(cmd), "curl.exe -s -f -o \"%s\" \"%s\" 2>nul", alias_file, url);
    if (system(cmd) != 0 || GetFileAttributesA(alias_file) == INVALID_FILE_ATTRIBUTES) {
        /* Try uma-get */
        snprintf(cmd, sizeof(cmd), "uma-get.exe -q -P \"%s\" --no-check-certificate \"%s\" 2>nul", alias_dir, url);
        if (system(cmd) != 0 || GetFileAttributesA(alias_file) == INVALID_FILE_ATTRIBUTES) {
            return 0;
        }
    }
    
    return 1;
}

/* Download metadata for package */
int download_metadata(const char *full_source_url, const char *package_name, const char *cache_dir) {
    char url[MAX_PATH_LEN];
    char cmd[MAX_CMD_LEN];
    char temp_file[MAX_PATH_LEN];
    char metadata_file[MAX_PATH_LEN];
    
    /* Build URL */
    snprintf(url, sizeof(url), "%s/%s.metadata", full_source_url, package_name);
    snprintf(temp_file, sizeof(temp_file), "%s\\%s.metadata", getenv("TEMP") ? getenv("TEMP") : "C:\\Windows\\Temp", package_name);
    snprintf(metadata_file, sizeof(metadata_file), "%s\\metadata.sque", cache_dir);
    
    /* Download using uma-get */
    snprintf(cmd, sizeof(cmd), "uma-get.exe -q -P \"%s\" --no-check-certificate \"%s\" >nul 2>nul", getenv("TEMP") ? getenv("TEMP") : "C:\\Windows\\Temp", url);
    if (system(cmd) != 0 || GetFileAttributesA(temp_file) == INVALID_FILE_ATTRIBUTES) {
        return 0;
    }
    
    /* Unzip metadata */
    snprintf(cmd, sizeof(cmd), "unzip.exe \"%s\" -d \"%s\" >nul 2>nul", temp_file, cache_dir);
    system(cmd);
    
    /* Delete temp file */
    DeleteFileA(temp_file);
    
    /* Check if metadata.sque exists */
    if (GetFileAttributesA(metadata_file) == INVALID_FILE_ATTRIBUTES) {
        return 0;
    }
    
    return 1;
}

/* Parse [ToUse] field from alias template */
int parse_alias_touse(const char *alias_file, char *package_name, int package_name_size) {
    return read_metadata_field(alias_file, "ToUse", package_name, package_name_size);
}

/* Read a field from sque file */
int read_metadata_field(const char *filepath, const char *fieldname, char *output, int output_size) {
    FILE *fp;
    char line[MAX_LINE];
    char field_tag[MAX_LINE];
    int in_field = 0;
    
    fp = fopen(filepath, "r");
    if (fp == NULL) {
        return 0;
    }
    
    snprintf(field_tag, sizeof(field_tag), "[%s]", fieldname);
    output[0] = '\0';
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        newline = strchr(line, '\r');
        if (newline) *newline = '\0';
        
        if (in_field) {
            if (line[0] == '[' && strchr(line, ']') != NULL) {
                break;
            }
            if (strlen(output) == 0 && strlen(line) > 0) {
                strncpy(output, line, output_size - 1);
                output[output_size - 1] = '\0';
                trim_whitespace(output);
                fclose(fp);
                return 1;
            }
        } else if (strcmpi(line, field_tag) == 0) {
            in_field = 1;
        }
    }
    
    fclose(fp);
    return 0;
}

/* Extract alias command from [Alias] section */
int extract_alias_section(const char *filepath, const char *alias_name, char *program, int program_size) {
    FILE *fp;
    char line[MAX_LINE];
    int in_alias = 0;
    
    fp = fopen(filepath, "r");
    if (fp == NULL) {
        return 0;
    }
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        newline = strchr(line, '\r');
        if (newline) *newline = '\0';
        
        if (in_alias) {
            if (line[0] == '[' || strcmp(line, "::end") == 0) {
                break;
            }
            if (strlen(line) > 0 && strchr(line, ':')) {
                char *colon = strchr(line, ':');
                char current_alias[MAX_LINE];
                
                strncpy(current_alias, line, colon - line);
                current_alias[colon - line] = '\0';
                trim_whitespace(current_alias);
                
                if (strcmpi(current_alias, alias_name) == 0) {
                    strncpy(program, colon + 1, program_size - 1);
                    program[program_size - 1] = '\0';
                    trim_whitespace(program);
                    fclose(fp);
                    return 1;
                }
            }
        } else if (strcmpi(line, "[Alias]") == 0) {
            in_alias = 1;
        }
    }
    
    fclose(fp);
    return 0;
}

/* Extract program from [DefaultOpen] section */
int extract_defaultopen(const char *metadata_file, const char *installdir, const char *pier_root, char *program_path, int program_path_size) {
    FILE *fp;
    char line[MAX_LINE];
    int in_default = 0;
    char default_prog[MAX_LINE];
    char first_prog[MAX_LINE];
    
    default_prog[0] = '\0';
    first_prog[0] = '\0';
    
    fp = fopen(metadata_file, "r");
    if (fp == NULL) {
        return 0;
    }
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        newline = strchr(line, '\r');
        if (newline) *newline = '\0';
        
        if (in_default) {
            if (line[0] == '[' || strcmp(line, "::end") == 0) {
                break;
            }
            if (strlen(line) > 0) {
                char test_path[MAX_PATH_LEN];
                int has_wildcard = 0;
                
                /* Check for wildcards */
                if (strchr(line, '*') || strchr(line, '?')) {
                    has_wildcard = 1;
                }
                
                if (has_wildcard) {
                    /* Use FindFirstFile for wildcards */
                    WIN32_FIND_DATAA findData;
                    HANDLE hFind;
                    char search_path[MAX_PATH_LEN];
                    
                    snprintf(search_path, sizeof(search_path), "%s\\app\\%s\\%s", pier_root, installdir, line);
                    hFind = FindFirstFileA(search_path, &findData);
                    if (hFind != INVALID_HANDLE_VALUE) {
                        snprintf(program_path, program_path_size, "%s\\app\\%s\\%s", pier_root, installdir, findData.cFileName);
                        FindClose(hFind);
                        fclose(fp);
                        return 1;
                    }
                } else {
                    /* Check if file exists */
                    snprintf(test_path, sizeof(test_path), "%s\\app\\%s\\%s", pier_root, installdir, line);
                    if (GetFileAttributesA(test_path) != INVALID_FILE_ATTRIBUTES) {
                        strncpy(program_path, test_path, program_path_size - 1);
                        program_path[program_path_size - 1] = '\0';
                        fclose(fp);
                        return 1;
                    }
                    /* Try with .exe */
                    strncat(test_path, ".exe", sizeof(test_path) - strlen(test_path) - 1);
                    if (GetFileAttributesA(test_path) != INVALID_FILE_ATTRIBUTES) {
                        strncpy(program_path, test_path, program_path_size - 1);
                        program_path[program_path_size - 1] = '\0';
                        fclose(fp);
                        return 1;
                    }
                }
            }
        } else if (strcmpi(line, "[DefaultOpen]") == 0) {
            in_default = 1;
        }
    }
    
    fclose(fp);
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
        /* Fallback to system() */
        system(cmd);
    }
}

/* Replace $1, $2, etc. placeholders with actual arguments */
void replace_placeholders(const char *template, char **user_args, int user_arg_count, int *used_args, char *output, int output_size) {
    const char *p = template;
    char *out = output;
    int out_len = 0;
    
    output[0] = '\0';
    
    while (*p && out_len < output_size - 1) {
        if (*p == '$' && *(p + 1) >= '1' && *(p + 1) <= '9') {
            int arg_num = *(p + 1) - '0';
            if (arg_num <= user_arg_count) {
                int arg_len = strlen(user_args[arg_num - 1]);
                if (out_len + arg_len < output_size - 1) {
                    strcpy(out, user_args[arg_num - 1]);
                    out += arg_len;
                    out_len += arg_len;
                    used_args[arg_num - 1] = 1;
                }
            }
            p += 2;
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
