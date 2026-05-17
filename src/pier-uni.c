/*
 * pier-uni.c - UTF-8 to console code page converter
 * Compatible with Windows XP and later, C89/C90
 * Reads from file or stdin, converts UTF-8 to console code page
 * 
 * Usage: pier-uni.exe <file>       - convert file
 *        pier-uni.exe -             - read from stdin
 *        some_command | pier-uni.exe  - pipe mode
 * Output: converted text on stdout
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAXCHUNK (1024 * 64)
#define OUTCHUNK (1024 * 16)

static int is_valid_utf8(const unsigned char *p, int len) {
    int i = 0;
    while (i < len && p[i]) {
        if (p[i] < 0x80) {
            i++;
        } else if ((p[i] & 0xE0) == 0xC0 && i + 1 < len) {
            if ((p[i+1] & 0xC0) != 0x80) return 0;
            i += 2;
        } else if ((p[i] & 0xF0) == 0xE0 && i + 2 < len) {
            if ((p[i+1] & 0xC0) != 0x80) return 0;
            if ((p[i+2] & 0xC0) != 0x80) return 0;
            i += 3;
        } else if ((p[i] & 0xF8) == 0xF0 && i + 3 < len) {
            if ((p[i+1] & 0xC0) != 0x80) return 0;
            if ((p[i+2] & 0xC0) != 0x80) return 0;
            if ((p[i+3] & 0xC0) != 0x80) return 0;
            i += 4;
        } else {
            return 0;
        }
    }
    return (i <= len) ? 1 : 0;
}

static int convert_and_write(const char *buf, int len, int is_utf8) {
    int wide_len;
    wchar_t *wide_str;
    int result;
    char *out_buf;

    if (!is_utf8) {
        fwrite(buf, 1, len, stdout);
        return 1;
    }

    wide_len = MultiByteToWideChar(CP_UTF8, 0, buf, len, NULL, 0);
    if (wide_len <= 0) {
        fwrite(buf, 1, len, stdout);
        return 1;
    }

    wide_str = (wchar_t *)malloc((wide_len + 1) * sizeof(wchar_t));
    if (!wide_str) {
        fwrite(buf, 1, len, stdout);
        return 0;
    }

    MultiByteToWideChar(CP_UTF8, 0, buf, len, wide_str, wide_len);
    wide_str[wide_len] = 0;

    result = WideCharToMultiByte(GetConsoleOutputCP(), 0, wide_str, -1, NULL, 0, NULL, NULL);
    if (result > 0) {
        out_buf = (char *)malloc(result + 1);
        if (out_buf) {
            WideCharToMultiByte(GetConsoleOutputCP(), 0, wide_str, -1, out_buf, result, NULL, NULL);
            fwrite(out_buf, 1, result - 1, stdout);
            free(out_buf);
        }
    } else {
        fwrite(buf, 1, len, stdout);
    }

    free(wide_str);
    return 1;
}

int main(int argc, char *argv[]) {
    const char *filepath = NULL;
    FILE *fp = NULL;
    int use_stdin = 0;
    unsigned char *buf;
    long sz;
    UINT console_cp;

    if (argc < 2) {
        use_stdin = 1;
    } else if (strcmp(argv[1], "-") == 0) {
        use_stdin = 1;
    } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        fprintf(stderr, "Usage: pier-uni [file|-]\n");
        fprintf(stderr, "  pier-uni <file>    Convert file (UTF-8 -> console CP)\n");
        fprintf(stderr, "  pier-uni -         Read from stdin\n");
        fprintf(stderr, "  some_cmd | pier-uni  Pipe mode\n");
        return 0;
    } else {
        filepath = argv[1];
    }

    console_cp = GetConsoleOutputCP();

    if (use_stdin) {
        size_t total = 0;
        size_t capacity = MAXCHUNK;
        buf = (unsigned char *)malloc(capacity);
        if (!buf) {
            fprintf(stderr, "Error: out of memory\n");
            return 1;
        }

        while (1) {
            size_t n = fread(buf + total, 1, capacity - total - 1, stdin);
            if (n == 0) break;
            total += n;
            if (total + MAXCHUNK >= capacity) {
                capacity += MAXCHUNK;
                unsigned char *newbuf = (unsigned char *)realloc(buf, capacity);
                if (!newbuf) break;
                buf = newbuf;
            }
        }
        buf[total] = 0;

        if (console_cp == 65001 || !is_valid_utf8(buf, (int)total)) {
            fwrite(buf, 1, total, stdout);
        } else {
            convert_and_write((const char *)buf, (int)total, 1);
        }

        free(buf);
        return 0;
    }

    {
        DWORD attr = GetFileAttributesA(filepath);
        if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            fprintf(stderr, "Error: file not found: %s\n", filepath);
            return 1;
        }
    }

    fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "Error: cannot open file\n");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (sz <= 0 || sz > MAXCHUNK) {
        fclose(fp);
        fprintf(stderr, "Error: file too large or empty (%ld bytes)\n", sz);
        return 1;
    }

    buf = (unsigned char *)malloc(sz + 1);
    if (!buf) {
        fclose(fp);
        fprintf(stderr, "Error: out of memory\n");
        return 1;
    }

    fread(buf, 1, sz, fp);
    buf[sz] = 0;
    fclose(fp);

    if (console_cp == 65001 || !is_valid_utf8(buf, (int)sz)) {
        fwrite(buf, 1, sz, stdout);
    } else {
        convert_and_write((const char *)buf, (int)sz, 1);
    }

    free(buf);
    return 0;
}