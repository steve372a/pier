/*
 * pier-upd.c - Pier self-updater
 * 完整的更新流程：下载 -> 解压 -> 复制
 * Compatible with C89/C90, Windows XP and later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_PATH_LEN 512
#define MAX_CMD_LEN 2048

void build_wget_proxy_opts(char *buf, int buf_size) {
    char *http_proxy, *https_proxy;
    buf[0] = '\0';
    http_proxy = getenv("http_proxy");
    if (http_proxy && http_proxy[0]) {
        snprintf(buf + strlen(buf), buf_size - strlen(buf), " -e http_proxy=%s", http_proxy);
    }
    https_proxy = getenv("https_proxy");
    if (https_proxy && https_proxy[0]) {
        snprintf(buf + strlen(buf), buf_size - strlen(buf), " -e https_proxy=%s", https_proxy);
    }
}

/* Copy file from src to dst */
int copy_file(const char *src, const char *dst) {
    FILE *fp_src, *fp_dst;
    char buffer[4096];
    size_t bytes_read;
    
    fp_src = fopen(src, "rb");
    if (!fp_src) return 0;
    
    fp_dst = fopen(dst, "wb");
    if (!fp_dst) {
        fclose(fp_src);
        return 0;
    }
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp_src)) > 0) {
        fwrite(buffer, 1, bytes_read, fp_dst);
    }
    
    fclose(fp_src);
    fclose(fp_dst);
    return 1;
}

/* Copy directory recursively */
int copy_dir(const char *src, const char *dst) {
    char src_path[MAX_PATH_LEN];
    char dst_path[MAX_PATH_LEN];
    WIN32_FIND_DATA find_data;
    HANDLE hFind;
    
    /* Create destination directory */
    CreateDirectoryA(dst, NULL);
    
    /* Find all files in source */
    snprintf(src_path, sizeof(src_path), "%s\\*", src);
    hFind = FindFirstFileA(src_path, &find_data);
    
    if (hFind == INVALID_HANDLE_VALUE) return 0;
    
    do {
        /* Skip . and .. */
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }
        
        snprintf(src_path, sizeof(src_path), "%s\\%s", src, find_data.cFileName);
        snprintf(dst_path, sizeof(dst_path), "%s\\%s", dst, find_data.cFileName);
        
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            /* Recursively copy subdirectory */
            copy_dir(src_path, dst_path);
        } else {
            /* Copy file */
            copy_file(src_path, dst_path);
        }
    } while (FindNextFileA(hFind, &find_data));
    
    FindClose(hFind);
    return 1;
}

/* Remove directory recursively */
void remove_dir(const char *path) {
    char search_path[MAX_PATH_LEN];
    char file_path[MAX_PATH_LEN];
    WIN32_FIND_DATA find_data;
    HANDLE hFind;
    
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    hFind = FindFirstFileA(search_path, &find_data);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
                continue;
            }
            
            snprintf(file_path, sizeof(file_path), "%s\\%s", path, find_data.cFileName);
            
            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                remove_dir(file_path);
                RemoveDirectoryA(file_path);
            } else {
                DeleteFileA(file_path);
            }
        } while (FindNextFileA(hFind, &find_data));
        
        FindClose(hFind);
    }
    
    RemoveDirectoryA(path);
}

/* Read language string from lang.ini */
int read_lang_string(const char *lang_file, const char *key, char *value, int max_len) {
    FILE *fp;
    char line[MAX_PATH_LEN];
    char section[MAX_PATH_LEN];
    int in_section = 0;
    
    fp = fopen(lang_file, "r");
    if (!fp) return -1;
    
    while (fgets(line, sizeof(line), fp)) {
        /* Remove newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        
        /* Check for section */
        if (line[0] == '[' && line[len-2] == ']') {
            strncpy(section, line + 1, len - 3);
            section[len-3] = '\0';
            if (strcmp(section, key) == 0) {
                in_section = 1;
            } else {
                in_section = 0;
            }
            continue;
        }
        
        /* Read value if in target section */
        if (in_section && line[0] != '\0' && line[0] != '[') {
            strncpy(value, line, max_len - 1);
            value[max_len - 1] = '\0';
            fclose(fp);
            return 0;
        }
    }
    
    fclose(fp);
    return -1;
}

int main(int argc, char *argv[]) {
    char pier_root[MAX_PATH_LEN];
    char lang_dir[MAX_PATH_LEN];
    char temp_exe[MAX_PATH_LEN];
    char temp_dir[MAX_PATH_LEN];
    char new_version[MAX_PATH_LEN];
    char lang_file[MAX_PATH_LEN];
    char src_path[MAX_PATH_LEN];
    char dst_path[MAX_PATH_LEN];
    char cmd[MAX_CMD_LEN];
    char str_progress[MAX_PATH_LEN];
    char str_downloading[MAX_PATH_LEN];
    char str_extracting[MAX_PATH_LEN];
    char str_complete[MAX_PATH_LEN];
    char str_failed[MAX_PATH_LEN];
    
    /* Get arguments */
    if (argc < 4) {
        printf("Usage: pier-upd.exe <PIER_ROOT> <LANGUAGE_DIR> <NEW_VERSION>\n");
        return 1;
    }
    
    strncpy(pier_root, argv[1], sizeof(pier_root) - 1);
    pier_root[sizeof(pier_root) - 1] = '\0';
    strncpy(lang_dir, argv[2], sizeof(lang_dir) - 1);
    lang_dir[sizeof(lang_dir) - 1] = '\0';
    strncpy(new_version, argv[3], sizeof(new_version) - 1);
    new_version[sizeof(new_version) - 1] = '\0';
    
    /* Build paths */
    snprintf(lang_file, sizeof(lang_file), "%s\\lang.ini", lang_dir);
    
    /* Read language strings */
    read_lang_string(lang_file, "update_pier_progress", str_progress, sizeof(str_progress));
    read_lang_string(lang_file, "update_pier_downloading", str_downloading, sizeof(str_downloading));
    read_lang_string(lang_file, "update_pier_extracting", str_extracting, sizeof(str_extracting));
    read_lang_string(lang_file, "update_pier_complete", str_complete, sizeof(str_complete));
    read_lang_string(lang_file, "update_pier_failed", str_failed, sizeof(str_failed));
    
    /* Get temp directory */
    GetTempPathA(sizeof(temp_dir), temp_dir);
    /* Remove trailing backslash if present */
    size_t temp_len = strlen(temp_dir);
    if (temp_len > 0 && temp_dir[temp_len - 1] == '\\') {
        temp_dir[temp_len - 1] = '\0';
    }
    
    /* Build temp exe path for cleanup */
    snprintf(temp_exe, sizeof(temp_exe), "%s\\pier-upd.exe", temp_dir);
    
    /* Perform full update */
    printf("%s\n", str_progress);
    
    /* Parent process (pier-ver.exe) has already exited */
    Sleep(1000);
    
    /* Step 1: Download */
    /* Delete existing zip file if present */
    {
        char zip_path[MAX_PATH_LEN];
        snprintf(zip_path, sizeof(zip_path), "%s\\pier-%s.zip", temp_dir, new_version);
        DeleteFileA(zip_path);
    }
    printf("%s%s...\n", str_downloading, new_version);
    {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        char pg_cmd[MAX_CMD_LEN];
        char zip_path[MAX_PATH_LEN];
        
        snprintf(zip_path, sizeof(zip_path), "%s\\pier-%s.zip", temp_dir, new_version);
        /* OLD (pier-get): snprintf(pg_cmd, sizeof(pg_cmd), "pier-get.exe \".\" \"https://github.com/steve372a/pier/releases/download/v%s/pier-%s.zip\" \"%s\"", new_version, new_version, zip_path); */
        {
            char proxy_opts[512];
            build_wget_proxy_opts(proxy_opts, sizeof(proxy_opts));
            snprintf(pg_cmd, sizeof(pg_cmd), "uma-get.exe -q --show-progress --timeout=14400 --tries=3 --no-check-certificate%s -O \"%s\" \"https://github.com/steve372a/pier/releases/download/v%s/pier-%s.zip\"",
                     proxy_opts, zip_path, new_version, new_version);
        }
        
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        
        if (!CreateProcessA(NULL, pg_cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            printf("%s\n", str_failed);
            system("pause");
            return 1;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        if (GetFileAttributesA(zip_path) == INVALID_FILE_ATTRIBUTES) {
            printf("%s\n", str_failed);
            system("pause");
            return 1;
        }
    }
    
    /* Step 2: Extract to pier-{version} subdirectory */
    printf("%s\n", str_extracting);
    {
        char extract_dir[MAX_PATH_LEN];
        char zip_path[MAX_PATH_LEN];
        snprintf(extract_dir, sizeof(extract_dir), "%s\\pier-%s", temp_dir, new_version);
        snprintf(zip_path, sizeof(zip_path), "%s\\pier-%s.zip", temp_dir, new_version);
        CreateDirectoryA(extract_dir, NULL);
        /* 7za x zipfile -o"outputdir" -y */
        snprintf(cmd, sizeof(cmd), "%s\\bin\\7za.exe x \"%s\" -o\"%s\" -y >nul 2>&1", 
                 pier_root, zip_path, extract_dir);
        system(cmd);
    }
    DeleteFileA(temp_exe);
    
    /* Step 3: Copy files */
    snprintf(src_path, sizeof(src_path), "%s\\pier-%s", temp_dir, new_version);
    snprintf(dst_path, sizeof(dst_path), "%s", pier_root);
    
    /* Debug: check if source exists */
    if (GetFileAttributesA(src_path) == INVALID_FILE_ATTRIBUTES) {
        printf("Source directory does not exist: %s\n", src_path);
        printf("Temp dir: %s\n", temp_dir);
        printf("New version: %s\n", new_version);
        system("pause");
        return 1;
    }
    
    if (!copy_dir(src_path, dst_path)) {
        printf("%s\n", str_failed);
        system("pause");
        return 1;
    }
    
    /* Clean up */
    remove_dir(src_path);
    DeleteFileA(temp_exe);
    
    printf("%s\n", str_complete);
    
    return 0;
}
