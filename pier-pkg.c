/*
 * pier-pkg-simple.c - Simplified version using direct file parsing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define MAX_PATH_LEN 1024
#define MAX_LINE 4096
#define MAX_NAME 256
#define MAX_PACKAGES 32

/* Global paths */
char g_pier_root[MAX_PATH_LEN];
char g_language_dir[MAX_PATH_LEN];
char g_source_url[MAX_PATH_LEN];
char g_pies_url[MAX_PATH_LEN];
char g_sys_arch[MAX_NAME];
int g_autoyes = 0;

/* Language strings - Install */
char g_str_loading[MAX_LINE];
char g_str_package_name[MAX_LINE];
char g_str_version[MAX_LINE];
char g_str_os_req[MAX_LINE];
char g_str_description[MAX_LINE];
char g_str_author[MAX_LINE];
char g_str_distributor[MAX_LINE];
char g_str_arch[MAX_LINE];
char g_str_confirm[MAX_LINE];
char g_str_downloading[MAX_LINE];
char g_str_installing[MAX_LINE];
char g_str_installed[MAX_LINE];
char g_str_error_not_found[MAX_LINE];

/* Language strings - Remove */
char g_str_choiceremove[MAX_LINE];
char g_str_uninstall_progress[MAX_LINE];
char g_str_uninstall_success[MAX_LINE];
char g_str_package_installed_info[MAX_LINE];
char g_str_remove_warning[MAX_LINE];
char g_str_remove_space_usage[MAX_LINE];
char g_str_remove_space_unit[MAX_LINE];
char g_str_package_not_installed[MAX_LINE];
char g_str_error_protected_lang[MAX_LINE];
char g_str_install_hint[MAX_LINE];

/* Language strings - Errors */
char g_str_error_package_not_found[MAX_LINE];
char g_str_error_no_packages[MAX_LINE];
char g_str_error_unknown_command[MAX_LINE];
char g_str_usage_general[MAX_LINE];
char g_str_usage_install[MAX_LINE];
char g_str_usage_remove[MAX_LINE];

/* Package info */
typedef struct {
    char name[MAX_NAME];
    char installer_name[MAX_NAME];
    char display_name[MAX_NAME];
    char version[MAX_NAME];
    char os_req[MAX_NAME];
    char description[MAX_LINE];
    char author[MAX_NAME];
    char distributor[MAX_NAME];
    char install_dir[MAX_PATH_LEN];
    char pie_file[MAX_NAME];
    char arch[MAX_NAME];
    double size_mb;
    char alias_list[MAX_LINE];
    int is_language;
    int metadata_downloaded;
} PackageInfo;

PackageInfo g_packages[MAX_PACKAGES];
int g_package_count = 0;

/* Check if file exists */
int file_exists(const char *path) {
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

/* Read string from lang.ini */
void read_lang_string(const char *key, char *value, int max_len) {
    char lang_file[MAX_PATH_LEN];
    FILE *fp;
    char line[MAX_LINE];
    char section[MAX_NAME];
    int in_section = 0;
    
    snprintf(lang_file, sizeof(lang_file), "%s\\lang.ini", g_language_dir);
    
    fp = fopen(lang_file, "r");
    if (!fp) {
        strncpy(value, key, max_len - 1);
        value[max_len - 1] = '\0';
        return;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        
        if (len == 0) continue;
        
        /* Check for section */
        if (line[0] == '[' && line[len-1] == ']') {
            strncpy(section, line + 1, sizeof(section) - 1);
            section[sizeof(section) - 1] = '\0';
            size_t sec_len = strlen(section);
            if (sec_len > 0 && section[sec_len-1] == ']') {
                section[sec_len-1] = '\0';
            }
            in_section = (strcmp(section, key) == 0);
            continue;
        }
        
        /* If in target section, read value */
        if (in_section) {
            strncpy(value, line, max_len - 1);
            value[max_len - 1] = '\0';
            fclose(fp);
            return;
        }
    }
    
    fclose(fp);
    strncpy(value, key, max_len - 1);
    value[max_len - 1] = '\0';
}

/* Print colored text using vecho.exe */
void color_printf(const char *color, const char *format, ...) {
    char short_root[MAX_PATH_LEN];
    char cmd[MAX_PATH_LEN * 4];
    char message[MAX_LINE];
    va_list args;
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    GetShortPathNameA(g_pier_root, short_root, sizeof(short_root));
    
    /* Use vecho.exe for colored output */
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe $%s$%s", short_root, color, message);
    system(cmd);
}

/* Print package info with colors like piec.bat */
void print_package_info_colored(int index) {
    char short_root[MAX_PATH_LEN];
    char cmd[MAX_PATH_LEN * 4];
    
    GetShortPathNameA(g_pier_root, short_root, sizeof(short_root));
    
    printf("\n");
    
    /* Package name: brightgreen label, brightwhite value */
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe $brightgreen$%s: $brightwhite$%s", 
             short_root, g_str_package_name, g_packages[index].display_name);
    system(cmd);
    
    /* Version: brightgreen label, brightwhite value */
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe $brightgreen$%s: $brightwhite$%s", 
             short_root, g_str_version, g_packages[index].version);
    system(cmd);
    
    /* OS Requirement: brightgreen label, brightwhite value */
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe $brightgreen$%s: $brightwhite$Windows %s", 
             short_root, g_str_os_req, g_packages[index].os_req);
    system(cmd);
    
    /* Description: brightyellow label, brightwhite value */
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe $brightyellow$%s: $brightwhite$%s", 
             short_root, g_str_description, g_packages[index].description);
    system(cmd);
    
    /* Author: brightyellow label, brightwhite value */
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe $brightyellow$%s: $brightwhite$%s", 
             short_root, g_str_author, g_packages[index].author);
    system(cmd);
    
    /* Distributor: brightyellow label, brightwhite value */
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe $brightyellow$%s: $brightwhite$%s", 
             short_root, g_str_distributor, g_packages[index].distributor);
    system(cmd);
    
    /* Architecture: brightgreen label, brightwhite value */
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe $brightgreen$%s: $brightwhite$%s", 
             short_root, g_str_arch, g_packages[index].arch);
    system(cmd);
}

/* Load all language strings */
void load_language(void) {
    /* Install strings */
    read_lang_string("loading_metadata", g_str_loading, sizeof(g_str_loading));
    read_lang_string("list_package_name", g_str_package_name, sizeof(g_str_package_name));
    read_lang_string("list_version", g_str_version, sizeof(g_str_version));
    read_lang_string("list_os_requirement", g_str_os_req, sizeof(g_str_os_req));
    read_lang_string("list_description", g_str_description, sizeof(g_str_description));
    read_lang_string("list_author", g_str_author, sizeof(g_str_author));
    read_lang_string("list_distributor", g_str_distributor, sizeof(g_str_distributor));
    read_lang_string("list_architecture", g_str_arch, sizeof(g_str_arch));
    read_lang_string("choiceapp", g_str_confirm, sizeof(g_str_confirm));
    read_lang_string("download_progress", g_str_downloading, sizeof(g_str_downloading));
    read_lang_string("install_progress", g_str_installing, sizeof(g_str_installing));
    read_lang_string("package_installed", g_str_installed, sizeof(g_str_installed));
    read_lang_string("error_package_not_exist", g_str_error_not_found, sizeof(g_str_error_not_found));
    
    /* Remove strings */
    read_lang_string("choiceremove", g_str_choiceremove, sizeof(g_str_choiceremove));
    read_lang_string("uninstall_progress", g_str_uninstall_progress, sizeof(g_str_uninstall_progress));
    read_lang_string("uninstall_success", g_str_uninstall_success, sizeof(g_str_uninstall_success));
    read_lang_string("package_installed", g_str_package_installed_info, sizeof(g_str_package_installed_info));
    read_lang_string("remove_warning_text", g_str_remove_warning, sizeof(g_str_remove_warning));
    read_lang_string("remove_space_usage", g_str_remove_space_usage, sizeof(g_str_remove_space_usage));
    read_lang_string("remove_space_usage_unit", g_str_remove_space_unit, sizeof(g_str_remove_space_unit));
    read_lang_string("package_not_installed", g_str_package_not_installed, sizeof(g_str_package_not_installed));
    read_lang_string("error_protected_lang", g_str_error_protected_lang, sizeof(g_str_error_protected_lang));
    read_lang_string("install_hint", g_str_install_hint, sizeof(g_str_install_hint));
    
    /* Error strings */
    read_lang_string("error_package_not_found", g_str_error_package_not_found, sizeof(g_str_error_package_not_found));
    read_lang_string("error_no_packages", g_str_error_no_packages, sizeof(g_str_error_no_packages));
    read_lang_string("error_unknown_command", g_str_error_unknown_command, sizeof(g_str_error_unknown_command));
    read_lang_string("usage_general", g_str_usage_general, sizeof(g_str_usage_general));
    read_lang_string("usage_install", g_str_usage_install, sizeof(g_str_usage_install));
    read_lang_string("usage_remove", g_str_usage_remove, sizeof(g_str_usage_remove));
}

/* Get architecture from metadata using pier-arch.exe */
void get_architecture(int index) {
    char metadata_file[MAX_PATH_LEN];
    char exe_path[MAX_PATH_LEN];
    char temp_file[MAX_PATH_LEN];
    char cmd[MAX_PATH_LEN * 4];
    FILE *fp;
    
    /* Use full path with GetFullPathName */
    char full_metadata[MAX_PATH_LEN];
    char full_exe[MAX_PATH_LEN];
    char full_temp[MAX_PATH_LEN];
    
    snprintf(metadata_file, sizeof(metadata_file), "%s\\share\\cache\\metadata.sque", g_pier_root);
    snprintf(exe_path, sizeof(exe_path), "%s\\bin\\pier-arch.exe", g_pier_root);
    snprintf(temp_file, sizeof(temp_file), "%s\\tmp_arch.txt", g_pier_root);
    
    GetFullPathNameA(metadata_file, MAX_PATH_LEN, full_metadata, NULL);
    GetFullPathNameA(exe_path, MAX_PATH_LEN, full_exe, NULL);
    GetFullPathNameA(temp_file, MAX_PATH_LEN, full_temp, NULL);
    
    /* Check if metadata file exists */
    if (!file_exists(full_metadata)) {
        strcpy(g_packages[index].arch, "all");
        return;
    }
    
    /* Call pier-arch.exe pkgarch using system for redirection */
    snprintf(cmd, sizeof(cmd), "\"%s\" pkgarch \"%s\" > \"%s\" 2>nul", full_exe, full_metadata, full_temp);
    system(cmd);
    
    fp = fopen(full_temp, "r");
    if (fp) {
        if (fgets(g_packages[index].arch, sizeof(g_packages[index].arch), fp)) {
            size_t len = strlen(g_packages[index].arch);
            if (len > 0 && g_packages[index].arch[len-1] == '\n') {
                g_packages[index].arch[len-1] = '\0';
            }
        }
        fclose(fp);
        remove(full_temp);
    }
    
    /* Fallback to "all" if empty */
    if (strlen(g_packages[index].arch) == 0) {
        strcpy(g_packages[index].arch, "all");
    }
}

/* Get PIE file name from metadata using pier-arch.exe */
void get_pie_file(int index) {
    char metadata_file[MAX_PATH_LEN];
    char exe_path[MAX_PATH_LEN];
    char temp_file[MAX_PATH_LEN];
    char cmd[MAX_PATH_LEN * 4];
    FILE *fp;
    
    /* Use full path with GetFullPathName */
    char full_metadata[MAX_PATH_LEN];
    char full_exe[MAX_PATH_LEN];
    char full_temp[MAX_PATH_LEN];
    
    snprintf(metadata_file, sizeof(metadata_file), "%s\\share\\cache\\metadata.sque", g_pier_root);
    snprintf(exe_path, sizeof(exe_path), "%s\\bin\\pier-arch.exe", g_pier_root);
    snprintf(temp_file, sizeof(temp_file), "%s\\tmp_pie.txt", g_pier_root);
    
    GetFullPathNameA(metadata_file, MAX_PATH_LEN, full_metadata, NULL);
    GetFullPathNameA(exe_path, MAX_PATH_LEN, full_exe, NULL);
    GetFullPathNameA(temp_file, MAX_PATH_LEN, full_temp, NULL);
    
    /* Check if metadata file exists */
    if (!file_exists(full_metadata)) {
        /* Fallback: use package name + .pie */
        snprintf(g_packages[index].pie_file, sizeof(g_packages[index].pie_file), "%s.pie", g_packages[index].name);
        return;
    }
    
    /* Call pier-arch.exe pkgfile using system for redirection */
    snprintf(cmd, sizeof(cmd), "\"%s\" pkgfile \"%s\" > \"%s\" 2>nul", full_exe, full_metadata, full_temp);
    system(cmd);
    
    fp = fopen(full_temp, "r");
    if (fp) {
        if (fgets(g_packages[index].pie_file, sizeof(g_packages[index].pie_file), fp)) {
            size_t len = strlen(g_packages[index].pie_file);
            if (len > 0 && g_packages[index].pie_file[len-1] == '\n') {
                g_packages[index].pie_file[len-1] = '\0';
            }
        }
        fclose(fp);
        remove(full_temp);
    }
    
    /* Fallback if empty */
    if (strlen(g_packages[index].pie_file) == 0) {
        snprintf(g_packages[index].pie_file, sizeof(g_packages[index].pie_file), "%s.pie", g_packages[index].name);
    }
}

/* Execute command and wait */
int execute_and_wait(const char *exe_path, const char *args) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmd_line[MAX_PATH_LEN * 4];
    
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    /* Build command line - exe_path already quoted if needed */
    snprintf(cmd_line, sizeof(cmd_line), "%s %s", exe_path, args);
    
    if (!CreateProcessA(NULL, cmd_line, NULL, NULL, FALSE, 
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return -1;
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return (int)exit_code;
}

/* Download metadata */
int download_metadata(const char *package_name, int index) {
    char cache_dir[MAX_PATH_LEN];
    char metadata_file[MAX_PATH_LEN];
    char exe_path[MAX_PATH_LEN];
    char args[MAX_PATH_LEN * 4];
    
    printf("%s\n", g_str_loading);
    
    /* Use short path names to avoid escape sequence issues */
    char short_root[MAX_PATH_LEN];
    GetShortPathNameA(g_pier_root, short_root, sizeof(short_root));
    
    /* Create cache directory */
    snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", short_root);
    CreateDirectoryA(cache_dir, NULL);
    
    /* Build metadata file path */
    snprintf(metadata_file, sizeof(metadata_file), "%s\\%s.metadata", cache_dir, package_name);
    
    /* Remove existing file */
    remove(metadata_file);
    
    /* Build uma-get command - use short paths without quotes for system() compatibility */
    snprintf(exe_path, sizeof(exe_path), "%s/bin/uma-get.exe", short_root);
    snprintf(args, sizeof(args), "-q -P %s --no-check-certificate \"%s/%s.metadata\"",
             cache_dir, g_source_url, package_name);
    
    /* Execute using system() for better compatibility */
    char cmd[MAX_PATH_LEN * 4];
    snprintf(cmd, sizeof(cmd), "%s %s", exe_path, args);
    system(cmd);
    
    /* Check if file was downloaded */
    if (!file_exists(metadata_file)) {
        printf("%s: %s\n", g_str_error_package_not_found, package_name);
        return 0;
    }
    
    /* Unzip metadata - hide output */
    snprintf(cmd, sizeof(cmd), "%s/bin/unzip.exe -o %s -d %s >nul 2>&1", short_root, metadata_file, cache_dir);
    system(cmd);
    
    g_packages[index].metadata_downloaded = 1;
    return 1;
}

/* Parse metadata file directly */
int parse_metadata(int index) {
    char metadata_file[MAX_PATH_LEN];
    FILE *fp;
    char line[MAX_LINE];
    char section[MAX_NAME];
    
    snprintf(metadata_file, sizeof(metadata_file), "%s\\share\\cache\\metadata.sque", g_pier_root);
    
    fp = fopen(metadata_file, "r");
    if (!fp) {
        return 0;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        /* Remove trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        
        /* Skip empty lines */
        if (len == 0) continue;
        
        /* Check for section header */
        if (line[0] == '[' && line[len-1] == ']') {
            strncpy(section, line + 1, sizeof(section) - 1);
            section[sizeof(section) - 1] = '\0';
            /* Remove trailing ] */
            size_t sec_len = strlen(section);
            if (sec_len > 0 && section[sec_len-1] == ']') {
                section[sec_len-1] = '\0';
            }
            continue;
        }
        
        /* Parse values based on section */
        if (strcmp(section, "PackageName") == 0) {
            strncpy(g_packages[index].display_name, line, sizeof(g_packages[index].display_name) - 1);
            g_packages[index].display_name[sizeof(g_packages[index].display_name) - 1] = '\0';
        } else if (strcmp(section, "InstallerName") == 0) {
            strncpy(g_packages[index].installer_name, line, sizeof(g_packages[index].installer_name) - 1);
            g_packages[index].installer_name[sizeof(g_packages[index].installer_name) - 1] = '\0';
        } else if (strcmp(section, "Version") == 0) {
            strncpy(g_packages[index].version, line, sizeof(g_packages[index].version) - 1);
            g_packages[index].version[sizeof(g_packages[index].version) - 1] = '\0';
        } else if (strcmp(section, "OS") == 0) {
            strncpy(g_packages[index].os_req, line, sizeof(g_packages[index].os_req) - 1);
            g_packages[index].os_req[sizeof(g_packages[index].os_req) - 1] = '\0';
            if (_stricmp(g_packages[index].os_req, "language") == 0) {
                g_packages[index].is_language = 1;
            }
        } else if (strcmp(section, "InstallDir") == 0) {
            strncpy(g_packages[index].install_dir, line, sizeof(g_packages[index].install_dir) - 1);
            g_packages[index].install_dir[sizeof(g_packages[index].install_dir) - 1] = '\0';
            /* Remove leading backslash */
            if (g_packages[index].install_dir[0] == '\\') {
                memmove(g_packages[index].install_dir, g_packages[index].install_dir + 1, strlen(g_packages[index].install_dir));
            }
        } else if (strcmp(section, "ProFile") == 0) {
            strncpy(g_packages[index].description, line, sizeof(g_packages[index].description) - 1);
            g_packages[index].description[sizeof(g_packages[index].description) - 1] = '\0';
        } else if (strcmp(section, "PackageSize") == 0) {
            g_packages[index].size_mb = atof(line);
        } else if (strcmp(section, "Author") == 0) {
            strncpy(g_packages[index].author, line, sizeof(g_packages[index].author) - 1);
            g_packages[index].author[sizeof(g_packages[index].author) - 1] = '\0';
        } else if (strcmp(section, "Distributor") == 0) {
            strncpy(g_packages[index].distributor, line, sizeof(g_packages[index].distributor) - 1);
            g_packages[index].distributor[sizeof(g_packages[index].distributor) - 1] = '\0';
        }
    }
    
    fclose(fp);
    
    /* Get architecture and PIE file */
    get_architecture(index);
    get_pie_file(index);
    
    return 1;
}

/* Display package info */
void display_package_info(int index) {
    print_package_info_colored(index);
}

/* Confirm installation */
int confirm_install(void) {
    char response[10];
    
    if (g_autoyes) {
        return 1;
    }
    
    printf("\n%s (Y/N): ", g_str_confirm);
    fflush(stdout);
    
    if (fgets(response, sizeof(response), stdin)) {
        if (response[0] == 'Y' || response[0] == 'y') {
            return 1;
        }
    }
    
    return 0;
}

/* Download package */
int download_package(int index) {
    char cache_dir[MAX_PATH_LEN];
    char pie_file[MAX_PATH_LEN];
    char exe_path[MAX_PATH_LEN];
    char args[MAX_PATH_LEN * 4];
    
    printf("%s %s...\n", g_str_downloading, g_packages[index].display_name);
    fflush(stdout);
    
    /* Use short path names to avoid escape sequence issues */
    char short_root[MAX_PATH_LEN];
    GetShortPathNameA(g_pier_root, short_root, sizeof(short_root));
    
    snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", short_root);
    snprintf(pie_file, sizeof(pie_file), "%s\\%s", cache_dir, g_packages[index].pie_file);
    remove(pie_file);
    
    /* Use system() for download to show progress properly */
    /* Convert backslashes to forward slashes in paths for system() */
    char cmd[MAX_PATH_LEN * 4];
    char path1[MAX_PATH_LEN], path2[MAX_PATH_LEN];
    int i;
    
    strncpy(path1, short_root, sizeof(path1) - 1);
    path1[sizeof(path1) - 1] = '\0';
    strncpy(path2, cache_dir, sizeof(path2) - 1);
    path2[sizeof(path2) - 1] = '\0';
    
    for (i = 0; path1[i]; i++) {
        if (path1[i] == '\\') path1[i] = '/';
    }
    for (i = 0; path2[i]; i++) {
        if (path2[i] == '\\') path2[i] = '/';
    }
    
    /* Use -q --show-progress to show only progress bar */
    snprintf(cmd, sizeof(cmd), "%s/bin/uma-get.exe -q --show-progress -P %s --no-check-certificate \"%s/%s\"",
             path1, path2, g_pies_url, g_packages[index].pie_file);
    system(cmd);
    
    /* Wait a bit for file to be written */
    Sleep(1000);
    
    if (!file_exists(pie_file)) {
        printf("%s: %s\n", g_str_error_not_found, g_packages[index].pie_file);
        return 0;
    }
    
    return 1;
}

/* Install package */
int install_package(int index) {
    char cache_dir[MAX_PATH_LEN];
    char pie_file[MAX_PATH_LEN];
    char app_dir[MAX_PATH_LEN];
    char exe_path[MAX_PATH_LEN];
    char args[MAX_PATH_LEN * 4];
    char cmd[MAX_PATH_LEN * 4];
    
    printf("%s %s...\n", g_str_installing, g_packages[index].display_name);
    
    Sleep(200);
    
    /* Use short path names to avoid escape sequence issues */
    char short_root[MAX_PATH_LEN];
    char short_cache[MAX_PATH_LEN];
    GetShortPathNameA(g_pier_root, short_root, sizeof(short_root));
    snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", short_root);
    snprintf(pie_file, sizeof(pie_file), "%s\\%s", cache_dir, g_packages[index].pie_file);
    
    if (g_packages[index].is_language) {
        if (_stricmp(g_packages[index].name, "zh-CN") == 0) {
            printf("%s\n", g_str_error_protected_lang);
            return 0;
        }
        
        snprintf(cmd, sizeof(cmd), "%s/bin/unzip.exe -o %s -d %s/share/language/ >nul 2>&1", short_root, pie_file, short_root);
        system(cmd);
        
        printf("%s\n", g_str_installed);
        return 1;
    }
    
    snprintf(app_dir, sizeof(app_dir), "%s\\app\\%s", short_root, g_packages[index].install_dir);
    CreateDirectoryA(app_dir, NULL);
    
    snprintf(cmd, sizeof(cmd), "%s/bin/unzip.exe -o %s -d %s >nul 2>&1", short_root, pie_file, app_dir);
    system(cmd);
    
    printf("%s: %s\n", g_str_installed, app_dir);
    
    /* Register in pierlist.sque */
    {
        char etc_dir[MAX_PATH_LEN];
        char pierlist_file[MAX_PATH_LEN];
        char metadata_dst[MAX_PATH_LEN];
        char cmd[MAX_PATH_LEN * 4];
        FILE *fp;
        
        /* Create etc directory */
        snprintf(etc_dir, sizeof(etc_dir), "%s\\etc", short_root);
        CreateDirectoryA(etc_dir, NULL);
        
        /* Create pierlist.sque if not exists */
        snprintf(pierlist_file, sizeof(pierlist_file), "%s\\pierlist.sque", etc_dir);
        if (!file_exists(pierlist_file)) {
            fp = fopen(pierlist_file, "w");
            if (fp) fclose(fp);
        }
        
        /* Remove existing entry using sed - use forward slashes */
        char pierlist_file_fwd[MAX_PATH_LEN];
        strncpy(pierlist_file_fwd, pierlist_file, sizeof(pierlist_file_fwd) - 1);
        pierlist_file_fwd[sizeof(pierlist_file_fwd) - 1] = '\0';
        int j;
        for (j = 0; pierlist_file_fwd[j]; j++) {
            if (pierlist_file_fwd[j] == '\\') pierlist_file_fwd[j] = '/';
        }
        snprintf(cmd, sizeof(cmd), "%s/bin/sed.exe -i \"/^%s | /d\" \"%s\" 2>nul", 
                 short_root, g_packages[index].installer_name, pierlist_file_fwd);
        system(cmd);
        
        /* Add new entry */
        fp = fopen(pierlist_file, "a");
        if (fp) {
            /* Format: InstallerName | Version | Date | InstallDir | Source */
            time_t now;
            struct tm *tm_info;
            char date_str[20];
            
            time(&now);
            tm_info = localtime(&now);
            snprintf(date_str, sizeof(date_str), "%04d.%02d.%02d", 
                     tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday);
            
            fprintf(fp, "%s | %s | %s | %s | %s\n", 
                    g_packages[index].installer_name,
                    g_packages[index].version,
                    date_str,
                    g_packages[index].install_dir,
                    g_source_url);
            fclose(fp);
        }
        
        /* Copy metadata to metadata directory - use forward slashes */
        char metadata_dst_fwd[MAX_PATH_LEN];
        snprintf(metadata_dst, sizeof(metadata_dst), "%s\\metadata", short_root);
        CreateDirectoryA(metadata_dst, NULL);
        snprintf(metadata_dst, sizeof(metadata_dst), "%s\\metadata\\%s.sque", 
                 short_root, g_packages[index].installer_name);
        strncpy(metadata_dst_fwd, metadata_dst, sizeof(metadata_dst_fwd) - 1);
        metadata_dst_fwd[sizeof(metadata_dst_fwd) - 1] = '\0';
        for (j = 0; metadata_dst_fwd[j]; j++) {
            if (metadata_dst_fwd[j] == '\\') metadata_dst_fwd[j] = '/';
        }
        snprintf(cmd, sizeof(cmd), "copy /y \"%s/share/cache/metadata.sque\" \"%s\" >nul 2>&1",
                 short_root, metadata_dst_fwd);
        system(cmd);
    }
    
    return 1;
}

/* Calculate folder size in MB */
double calculate_folder_size(const char *path) {
    WIN32_FIND_DATAA findData;
    HANDLE hFind;
    char search_path[MAX_PATH_LEN];
    LARGE_INTEGER totalSize;
    totalSize.QuadPart = 0;
    
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    hFind = FindFirstFileA(search_path, &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    char subdir[MAX_PATH_LEN];
                    snprintf(subdir, sizeof(subdir), "%s\\%s", path, findData.cFileName);
                    totalSize.QuadPart += (LONGLONG)(calculate_folder_size(subdir) * 1048576);
                } else {
                    LARGE_INTEGER fileSize;
                    fileSize.LowPart = findData.nFileSizeLow;
                    fileSize.HighPart = findData.nFileSizeHigh;
                    totalSize.QuadPart += fileSize.QuadPart;
                }
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
    
    return (double)totalSize.QuadPart / 1048576.0;
}

/* Print package info with colors for remove */
void print_package_info_remove_colored(int index, double folder_size) {
    char short_root[MAX_PATH_LEN];
    char cmd[MAX_PATH_LEN * 4];
    char size_str[64];
    
    GetShortPathNameA(g_pier_root, short_root, sizeof(short_root));
    
    printf("\n");
    
    /* Package name: brightgreen label, brightwhite value - same format as install */
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe $brightgreen$%s: $brightwhite$%s", 
             short_root, g_str_package_name, g_packages[index].display_name);
    system(cmd);
    
    /* Version: brightgreen label, brightwhite value */
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe $brightgreen$%s: $brightwhite$%s", 
             short_root, g_str_version, g_packages[index].version);
    system(cmd);
    
    /* Space usage: lang strings already contain color codes */
    snprintf(size_str, sizeof(size_str), "%.1f", folder_size);
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe %s: $brightred$%s %s", 
             short_root, g_str_remove_space_usage, size_str, g_str_remove_space_unit);
    system(cmd);
}

/* Print warning with colors for remove */
void print_remove_warning_colored(void) {
    char short_root[MAX_PATH_LEN];
    char cmd[MAX_PATH_LEN * 4];
    
    GetShortPathNameA(g_pier_root, short_root, sizeof(short_root));
    
    printf("\n");
    
    /* Use g_str_remove_warning which already contains color codes from lang.ini */
    snprintf(cmd, sizeof(cmd), "%s/bin/vecho.exe %s", short_root, g_str_remove_warning);
    system(cmd);
    
    printf("\n");
}

/* Remove package */
int remove_package(int index) {
    char target_dir[MAX_PATH_LEN];
    char short_root[MAX_PATH_LEN];
    char cmd[MAX_PATH_LEN * 4];
    int is_installed = 0;
    double folder_size = 0.0;
    
    /* Use short path names */
    GetShortPathNameA(g_pier_root, short_root, sizeof(short_root));
    
    /* Check if installed */
    if (g_packages[index].is_language) {
        snprintf(target_dir, sizeof(target_dir), "%s\\share\\language\\%s", short_root, g_packages[index].name);
    } else {
        snprintf(target_dir, sizeof(target_dir), "%s\\app\\%s", short_root, g_packages[index].install_dir);
    }
    
    if (file_exists(target_dir)) {
        is_installed = 1;
        folder_size = calculate_folder_size(target_dir);
    }
    
    if (!is_installed) {
        char vecho_cmd[MAX_PATH_LEN * 4];
        color_printf("brightred", g_str_package_not_installed);
        /* Show install hint with highlighted command */
        snprintf(vecho_cmd, sizeof(vecho_cmd), "%s/bin/vecho.exe %s $brightyellow$pier install %s$write$", 
                 short_root, g_str_install_hint, g_packages[index].name);
        system(vecho_cmd);
        return 0;
    }
    
    /* Display package info with colors */
    print_package_info_remove_colored(index, folder_size);
    
    /* Display warning with colors */
    print_remove_warning_colored();
    
    /* Confirm */
    if (!g_autoyes) {
        char response[10];
        printf("%s %s (Y/N): ", g_str_choiceremove, g_packages[index].display_name);
        fflush(stdout);
        if (fgets(response, sizeof(response), stdin)) {
            if (response[0] != 'Y' && response[0] != 'y') {
                return 0;  /* User cancelled */
            }
        }
    }
    
    /* Uninstall */
    printf("%s %s...\n", g_str_uninstall_progress, g_packages[index].display_name);
    
    /* Check protected language */
    if (g_packages[index].is_language && _stricmp(g_packages[index].name, "zh-CN") == 0) {
        printf("%s\n", g_str_error_protected_lang);
        return 0;
    }
    
    /* Run uninstall.exe if exists */
    if (!g_packages[index].is_language) {
        char uninstall_exe[MAX_PATH_LEN];
        snprintf(uninstall_exe, sizeof(uninstall_exe), "%s\\uninstall.exe", target_dir);
        if (file_exists(uninstall_exe)) {
            char uninstall_cmd[MAX_PATH_LEN * 2];
            snprintf(uninstall_cmd, sizeof(uninstall_cmd), "\"%s\"", uninstall_exe);
            system(uninstall_cmd);
        }
    }
    
    /* Remove directory */
    if (file_exists(target_dir)) {
        snprintf(cmd, sizeof(cmd), "rd /s /q \"%s\" >nul 2>&1", target_dir);
        system(cmd);
    }
    
    /* Remove metadata */
    char metadata_file[MAX_PATH_LEN];
    snprintf(metadata_file, sizeof(metadata_file), "%s\\metadata\\%s.sque", short_root, g_packages[index].installer_name);
    if (file_exists(metadata_file)) {
        remove(metadata_file);
    }
    
    /* Remove from pierlist.sque */
    char pierlist_file[MAX_PATH_LEN];
    char pierlist_file_fwd[MAX_PATH_LEN];
    int j;
    snprintf(pierlist_file, sizeof(pierlist_file), "%s\\etc\\pierlist.sque", short_root);
    if (file_exists(pierlist_file)) {
        strncpy(pierlist_file_fwd, pierlist_file, sizeof(pierlist_file_fwd) - 1);
        pierlist_file_fwd[sizeof(pierlist_file_fwd) - 1] = '\0';
        for (j = 0; pierlist_file_fwd[j]; j++) {
            if (pierlist_file_fwd[j] == '\\') pierlist_file_fwd[j] = '/';
        }
        snprintf(cmd, sizeof(cmd), "%s/bin/sed.exe -i \"/^%s | /d\" \"%s\" 2>nul", 
                 short_root, g_packages[index].installer_name, pierlist_file_fwd);
        system(cmd);
    }
    
    printf("%s\n", g_str_uninstall_success);
    
    return 1;
}

/* Main function */
int main(int argc, char *argv[]) {
    int i;
    int is_install = 0;
    
    if (argc < 2) {
        fprintf(stderr, "pier-pkg: install|remove <args...>\n");
        return 1;
    }
    
    if (_stricmp(argv[1], "install") == 0) {
        is_install = 1;
    } else if (_stricmp(argv[1], "remove") == 0) {
        is_install = 0;
    } else {
        fprintf(stderr, "pier-pkg: unknown command %s\n", argv[1]);
        return 1;
    }
    
    if (is_install && argc < 9) {
        fprintf(stderr, "pier-pkg: install <PIER_ROOT> <LANGUAGE_DIR> <source_url> <pies_url> <autoyes> <SYS_ARCH> <package>...\n");
        return 1;
    }
    
    if (!is_install && argc < 7) {
        fprintf(stderr, "pier-pkg: remove <PIER_ROOT> <LANGUAGE_DIR> <source_url> <autoyes> <package>...\n");
        return 1;
    }
    
    /* Parse arguments */
    strncpy(g_pier_root, argv[2], sizeof(g_pier_root) - 1);
    g_pier_root[sizeof(g_pier_root) - 1] = '\0';
    
    strncpy(g_language_dir, argv[3], sizeof(g_language_dir) - 1);
    g_language_dir[sizeof(g_language_dir) - 1] = '\0';
    
    strncpy(g_source_url, argv[4], sizeof(g_source_url) - 1);
    g_source_url[sizeof(g_source_url) - 1] = '\0';
    
    if (is_install) {
        strncpy(g_pies_url, argv[5], sizeof(g_pies_url) - 1);
        g_pies_url[sizeof(g_pies_url) - 1] = '\0';
        
        if (_stricmp(argv[6], "-y") == 0 || _stricmp(argv[6], "y") == 0 || _stricmp(argv[6], "yes") == 0) {
            g_autoyes = 1;
        }
        
        strncpy(g_sys_arch, argv[7], sizeof(g_sys_arch) - 1);
        g_sys_arch[sizeof(g_sys_arch) - 1] = '\0';
        
        for (i = 8; i < argc && g_package_count < MAX_PACKAGES; i++) {
            strncpy(g_packages[g_package_count].name, argv[i], sizeof(g_packages[g_package_count].name) - 1);
            g_packages[g_package_count].name[sizeof(g_packages[g_package_count].name) - 1] = '\0';
            g_package_count++;
        }
    } else {
        if (_stricmp(argv[5], "-y") == 0 || _stricmp(argv[5], "y") == 0 || _stricmp(argv[5], "yes") == 0) {
            g_autoyes = 1;
        }
        
        for (i = 6; i < argc && g_package_count < MAX_PACKAGES; i++) {
            strncpy(g_packages[g_package_count].name, argv[i], sizeof(g_packages[g_package_count].name) - 1);
            g_packages[g_package_count].name[sizeof(g_packages[g_package_count].name) - 1] = '\0';
            g_package_count++;
        }
    }
    
    if (g_package_count == 0) {
        fprintf(stderr, "pier-pkg: no packages specified\n");
        return 1;
    }
    
    /* Load language strings */
    load_language();
    
    /* Process packages */
    if (is_install) {
        /* Download all metadata */
        for (i = 0; i < g_package_count; i++) {
            if (!download_metadata(g_packages[i].name, i)) {
                continue;
            }
            if (!parse_metadata(i)) {
                continue;
            }
        }
        
        /* Display info */
        for (i = 0; i < g_package_count; i++) {
            if (g_packages[i].metadata_downloaded) {
                display_package_info(i);
            }
        }
        
        /* Confirm */
        if (!confirm_install()) {
            return 2;
        }
        
        /* Download and install */
        for (i = 0; i < g_package_count; i++) {
            if (!g_packages[i].metadata_downloaded) {
                continue;
            }
            
            if (!download_package(i)) {
                continue;
            }
            
            install_package(i);
        }
        
        /* Cleanup cache after installation */
        {
            char cache_dir[MAX_PATH_LEN];
            char cmd[MAX_PATH_LEN * 4];
            snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", g_pier_root);
            snprintf(cmd, sizeof(cmd), "rd /s /q \"%s\" >nul 2>&1", cache_dir);
            system(cmd);
        }
    } else {
        /* Remove mode */
        /* Download all metadata */
        for (i = 0; i < g_package_count; i++) {
            if (!download_metadata(g_packages[i].name, i)) {
                continue;
            }
            if (!parse_metadata(i)) {
                continue;
            }
        }
        
        /* Remove packages */
        for (i = 0; i < g_package_count; i++) {
            if (!g_packages[i].metadata_downloaded) {
                continue;
            }
            
            remove_package(i);
        }
        
        /* Cleanup cache after removal */
        {
            char cache_dir[MAX_PATH_LEN];
            char cmd[MAX_PATH_LEN * 4];
            snprintf(cache_dir, sizeof(cache_dir), "%s\\share\\cache", g_pier_root);
            snprintf(cmd, sizeof(cmd), "rd /s /q \"%s\" >nul 2>&1", cache_dir);
            system(cmd);
        }
    }
    
    return 0;
}
