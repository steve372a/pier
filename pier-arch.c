/*
 * pier-arch.c - Architecture detection and metadata parser for Pier
 * Compatible with C89/C90, Windows XP and later
 * Sanakaprix (https://steve372a.github.io)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024
#define MAX_PATH_LEN 512

/* Function prototypes */
int get_system_arch(void);
int get_system_arch_name(char *output, int output_size);
int read_metadata_field(const char *filepath, const char *fieldname, char *output, int output_size);
int parse_url_section(const char *filepath, const char *sysarch, char *filename, int filename_size);
void extract_filename(char *str);
void trim_whitespace(char *str);
void remove_default_tag(char *str);
int strcmpi(const char *s1, const char *s2);
int starts_with(const char *str, const char *prefix);

/* Main function */
int main(int argc, char *argv[])
{
    char result[MAX_LINE];
    char filepath[MAX_PATH_LEN];
    
    /* Check arguments */
    if (argc < 2) {
        printf("error: no command specified\n");
        return 1;
    }
    
    /* Command: sysarch - detect system architecture */
    if (strcmp(argv[1], "sysarch") == 0) {
        if (get_system_arch()) {
            return 0;
        } else {
            printf("error: cannot detect architecture\n");
            return 1;
        }
    }
    
    /* Commands that need metadata file */
    if (argc < 3) {
        printf("error: metadata file path required\n");
        return 1;
    }
    
    strncpy(filepath, argv[2], MAX_PATH_LEN - 1);
    filepath[MAX_PATH_LEN - 1] = '\0';
    
    /* Command: pkgarch - read package architecture */
    if (strcmp(argv[1], "pkgarch") == 0) {
        if (read_metadata_field(filepath, "Architecture", result, MAX_LINE)) {
            /* If empty or not found, default to "all" */
            if (strlen(result) == 0) {
                strcpy(result, "all");
            }
            printf("%s\n", result);
            return 0;
        } else {
            /* Field not found, default to all */
            printf("all\n");
            return 0;
        }
    }
    
    /* Command: pkgfile - get filename to download */
    if (strcmp(argv[1], "pkgfile") == 0) {
        char sysarch[16];
        
        /* Get system architecture first */
        if (!get_system_arch_name(sysarch, sizeof(sysarch))) {
            strcpy(sysarch, "x86");
        }
        
        /* Parse URL section */
        if (parse_url_section(filepath, sysarch, result, MAX_LINE)) {
            printf("%s\n", result);
            return 0;
        } else {
            printf("error: cannot determine filename\n");
            return 1;
        }
    }
    
    /* Command: check - check compatibility */
    if (strcmp(argv[1], "check") == 0) {
        char sysarch[16];
        char pkgarch[16];
        
        /* Get system architecture */
        if (!get_system_arch_name(sysarch, sizeof(sysarch))) {
            strcpy(sysarch, "x86");
        }
        
        /* Get package architecture */
        if (read_metadata_field(filepath, "Architecture", pkgarch, sizeof(pkgarch))) {
            if (strlen(pkgarch) == 0) {
                strcpy(pkgarch, "all");
            }
        } else {
            strcpy(pkgarch, "all");
        }
        
        /* Check compatibility */
        if (strcmp(pkgarch, "all") == 0 || 
            strcmpi(pkgarch, sysarch) == 0) {
            printf("ok\n");
        } else {
            printf("warn:%s\n", pkgarch);
        }
        return 0;
    }
    
    printf("error: unknown command %s\n", argv[1]);
    return 1;
}

/* Helper function to get system architecture name */
int get_system_arch_name(char *output, int output_size)
{
    const char *arch = getenv("PROCESSOR_ARCHITECTURE");
    const char *archw6432 = getenv("PROCESSOR_ARCHITEW6432");
    
    if (arch == NULL) {
        return 0;
    }
    
    if (strcmpi(arch, "AMD64") == 0) {
        strncpy(output, "x64", output_size - 1);
        output[output_size - 1] = '\0';
        return 1;
    }
    
    if (strcmpi(arch, "ARM64") == 0) {
        strncpy(output, "arm64", output_size - 1);
        output[output_size - 1] = '\0';
        return 1;
    }
    
    if (strcmpi(arch, "x86") == 0) {
        if (archw6432 != NULL && strlen(archw6432) > 0) {
            /* 64-bit Windows running 32-bit cmd */
            strncpy(output, "x64", output_size - 1);
        } else {
            strncpy(output, "x86", output_size - 1);
        }
        output[output_size - 1] = '\0';
        return 1;
    }
    
    /* Default to x86 */
    strncpy(output, "x86", output_size - 1);
    output[output_size - 1] = '\0';
    return 1;
}

/* Detect system architecture and print it */
int get_system_arch(void)
{
    char arch[16];
    if (get_system_arch_name(arch, sizeof(arch))) {
        printf("%s\n", arch);
        return 1;
    }
    return 0;
}

/* Read a field from metadata file */
int read_metadata_field(const char *filepath, const char *fieldname, char *output, int output_size)
{
    FILE *fp;
    char line[MAX_LINE];
    char field_tag[MAX_LINE];
    int found = 0;
    int in_field = 0;
    
    fp = fopen(filepath, "r");
    if (fp == NULL) {
        return 0;
    }
    
    /* Create field tag like [Architecture] */
    snprintf(field_tag, sizeof(field_tag), "[%s]", fieldname);
    
    output[0] = '\0';
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        /* Remove newline */
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        newline = strchr(line, '\r');
        if (newline) *newline = '\0';
        
        if (in_field) {
            /* Check if we hit next field */
            if (line[0] == '[' && strchr(line, ']') != NULL) {
                break;
            }
            /* Skip empty lines at start */
            if (strlen(output) == 0 && strlen(line) == 0) {
                continue;
            }
            /* Get first non-empty line as value */
            if (strlen(output) == 0 && strlen(line) > 0) {
                strncpy(output, line, output_size - 1);
                output[output_size - 1] = '\0';
                trim_whitespace(output);
                found = 1;
                break;
            }
        } else if (strcmpi(line, field_tag) == 0) {
            in_field = 1;
        }
    }
    
    fclose(fp);
    return found;
}

/* Parse URL section to get filename for architecture */
int parse_url_section(const char *filepath, const char *sysarch, char *filename, int filename_size)
{
    FILE *fp;
    char line[MAX_LINE];
    int in_url = 0;
    int is_multiarch = 0;
    char default_file[MAX_LINE];
    char first_file[MAX_LINE];
    char arch_line[MAX_LINE];
    
    fp = fopen(filepath, "r");
    if (fp == NULL) {
        return 0;
    }
    
    default_file[0] = '\0';
    first_file[0] = '\0';
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        /* Remove newline */
        char *newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        newline = strchr(line, '\r');
        if (newline) *newline = '\0';
        
        if (in_url) {
            /* Check for end marker */
            if (strcmp(line, "::end") == 0) {
                is_multiarch = 1;
                break;
            }
            
            /* Check if we hit next field */
            if (line[0] == '[' && strchr(line, ']') != NULL) {
                break;
            }
            
            /* Skip empty lines */
            if (strlen(line) == 0) {
                continue;
            }
            
            /* Check for architecture prefix (x86:, x64:, arm64:) */
            snprintf(arch_line, sizeof(arch_line), "%s:", sysarch);
            if (starts_with(line, arch_line)) {
                /* Found matching architecture */
                char *colon = strchr(line, ':');
                if (colon != NULL) {
                    strncpy(filename, colon + 1, filename_size - 1);
                    filename[filename_size - 1] = '\0';
                    trim_whitespace(filename);
                    remove_default_tag(filename);
                    extract_filename(filename);  /* Extract just the filename */
                    fclose(fp);
                    return 1;
                }
            }
            
            /* Check for (default) marker */
            if (strstr(line, "(default)") != NULL) {
                char *colon = strchr(line, ':');
                if (colon != NULL && strlen(default_file) == 0) {
                    strncpy(default_file, colon + 1, sizeof(default_file) - 1);
                    default_file[sizeof(default_file) - 1] = '\0';
                    trim_whitespace(default_file);
                }
            }
            
            /* Save first valid line */
            if (strlen(first_file) == 0 && strchr(line, ':') != NULL) {
                char *colon = strchr(line, ':');
                strncpy(first_file, colon + 1, sizeof(first_file) - 1);
                first_file[sizeof(first_file) - 1] = '\0';
                trim_whitespace(first_file);
            }
        } else if (strcmpi(line, "[URL]") == 0) {
            in_url = 1;
        }
    }
    
    fclose(fp);
    
    /* If we found ::end, it's multi-arch format */
    if (is_multiarch) {
        /* Try default first */
        if (strlen(default_file) > 0) {
            strncpy(filename, default_file, filename_size - 1);
            filename[filename_size - 1] = '\0';
            remove_default_tag(filename);
            extract_filename(filename);  /* Extract just the filename */
            return 1;
        }
        /* Then try first file */
        if (strlen(first_file) > 0) {
            strncpy(filename, first_file, filename_size - 1);
            filename[filename_size - 1] = '\0';
            remove_default_tag(filename);
            extract_filename(filename);  /* Extract just the filename */
            return 1;
        }
    } else {
        /* Single line format - read [URL] section as single value */
        /* Re-read to get simple URL */
        fp = fopen(filepath, "r");
        if (fp != NULL) {
            in_url = 0;
            while (fgets(line, sizeof(line), fp) != NULL) {
                char *newline = strchr(line, '\n');
                if (newline) *newline = '\0';
                newline = strchr(line, '\r');
                if (newline) *newline = '\0';
                
                if (in_url) {
                    if (line[0] == '[' && strchr(line, ']') != NULL) {
                        break;
                    }
                    if (strlen(line) > 0) {
                        strncpy(filename, line, filename_size - 1);
                        filename[filename_size - 1] = '\0';
                        trim_whitespace(filename);
                        extract_filename(filename);  /* Extract just the filename */
                        fclose(fp);
                        return 1;
                    }
                } else if (strcmpi(line, "[URL]") == 0) {
                    in_url = 1;
                }
            }
            fclose(fp);
        }
    }
    
    return 0;
}

/* Extract filename from path (get last component after / or \) */
void extract_filename(char *str)
{
    char *last_slash = NULL;
    char *last_backslash = NULL;
    
    /* Find last slash or backslash */
    last_slash = strrchr(str, '/');
    last_backslash = strrchr(str, '\\');
    
    if (last_slash != NULL && last_backslash != NULL) {
        /* Both found, use the later one */
        if (last_slash > last_backslash) {
            memmove(str, last_slash + 1, strlen(last_slash + 1) + 1);
        } else {
            memmove(str, last_backslash + 1, strlen(last_backslash + 1) + 1);
        }
    } else if (last_slash != NULL) {
        memmove(str, last_slash + 1, strlen(last_slash + 1) + 1);
    } else if (last_backslash != NULL) {
        memmove(str, last_backslash + 1, strlen(last_backslash + 1) + 1);
    }
    /* If no slash found, str is already the filename */
}

/* Trim whitespace from both ends */
void trim_whitespace(char *str)
{
    char *start = str;
    char *end;
    
    /* Skip leading spaces */
    while (*start == ' ' || *start == '\t') {
        start++;
    }
    
    /* Move trimmed string to beginning */
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    
    /* Trim trailing spaces */
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
}

/* Remove (default) tag from filename */
void remove_default_tag(char *str)
{
    char *tag = strstr(str, "(default)");
    if (tag != NULL) {
        /* Remove the tag */
        memmove(tag, tag + 9, strlen(tag + 9) + 1);
        trim_whitespace(str);
    }
}

/* Case-insensitive string compare */
int strcmpi(const char *s1, const char *s2)
{
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        
        /* Convert to lowercase */
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

/* Check if string starts with prefix */
int starts_with(const char *str, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    size_t str_len = strlen(str);
    
    if (str_len < prefix_len) {
        return 0;
    }
    
    return (strncmp(str, prefix, prefix_len) == 0);
}
