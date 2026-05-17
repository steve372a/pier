#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "sque.h"

#define MAX_LINE 4096

/* ---- Internal UTF-8 utilities ---- */

static int is_valid_utf8(const unsigned char *p)
{
    while (*p) {
        if (*p < 0x80) {
            p++;
        } else if ((*p & 0xE0) == 0xC0) {
            if ((p[1] & 0xC0) != 0x80) return 0;
            p += 2;
        } else if ((*p & 0xF0) == 0xE0) {
            if ((p[1] & 0xC0) != 0x80) return 0;
            if ((p[2] & 0xC0) != 0x80) return 0;
            p += 3;
        } else if ((*p & 0xF8) == 0xF0) {
            if ((p[1] & 0xC0) != 0x80) return 0;
            if ((p[2] & 0xC0) != 0x80) return 0;
            if ((p[3] & 0xC0) != 0x80) return 0;
            p += 4;
        } else {
            return 0;
        }
    }
    return 1;
}

void sque_utf8_to_acp(char *str, int max_len)
{
    int wide_len;
    wchar_t *wide_str;
    int acp_len;

    if (!str || !str[0]) return;
    if (max_len <= 1) return;

    if (!is_valid_utf8((const unsigned char *)str)) return;

    wide_len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (wide_len <= 0) return;

    wide_str = (wchar_t *)malloc(wide_len * sizeof(wchar_t));
    if (!wide_str) return;

    if (MultiByteToWideChar(CP_UTF8, 0, str, -1, wide_str, wide_len) <= 0) {
        free(wide_str);
        return;
    }

    acp_len = WideCharToMultiByte(CP_ACP, 0, wide_str, -1, NULL, 0, NULL, NULL);
    if (acp_len > 0 && acp_len <= max_len) {
        WideCharToMultiByte(CP_ACP, 0, wide_str, -1, str, acp_len, NULL, NULL);
    }

    free(wide_str);
}

/* ---- Internal helpers ---- */

/* Custom line reader that handles \r, \n, and \r\n line endings.
 * Standard fgets only handles \n, which fails on Mac-style \r-only files.
 * Returns line (without any line-ending chars), or NULL on EOF. */
static char *sque_fgets(char *buf, int size, FILE *fp)
{
    int c, pos = 0;
    buf[0] = '\0';
    if (feof(fp)) return NULL;
    while (pos < size - 1) {
        c = fgetc(fp);
        if (c == EOF) {
            if (pos == 0) return NULL;
            break;
        }
        if (c == '\r') {
            c = fgetc(fp);
            if (c != '\n' && c != EOF) ungetc(c, fp);
            break;
        }
        if (c == '\n') break;
        buf[pos++] = (char)c;
    }
    buf[pos] = '\0';
    return buf;
}

static int match_key(const char *line, const char *key)
{
    int klen = (int)strlen(key);
    if (line[0] != '[') return 0;
    if (_strnicmp(line + 1, key, klen) != 0) return 0;
    if (line[1 + klen] != ']') return 0;
    return 1;
}

static int is_section_line(const char *line)
{
    return (line[0] == '[' && strchr(line, ']') != NULL);
}

/* ---- Public API ---- */

int sque_read(const char *filepath, const char *key,
              char *value_buf, int bufsize)
{
    FILE *fp;
    char line[MAX_LINE];
    int in_field = 0;
    int pos = 0;

    value_buf[0] = '\0';

    fp = fopen(filepath, "r");
    if (!fp) return -1;

    while (sque_fgets(line, sizeof(line), fp) != NULL) {

        if (in_field) {
            if (strcmp(line, "::end") == 0) break;
            if (is_section_line(line)) break;
            if (strlen(line) > 0) {
                if (pos > 0 && pos + 1 < bufsize) {
                    value_buf[pos++] = '\n';
                }
                if (pos + (int)strlen(line) < bufsize) {
                    strcpy(value_buf + pos, line);
                    pos += (int)strlen(line);
                }
            }
            continue;
        }

        if (match_key(line, key)) {
            in_field = 1;
        }
    }

    fclose(fp);

    if (pos > 0) {
        value_buf[pos] = '\0';
        sque_utf8_to_acp(value_buf, bufsize);
        return pos;
    }
    return -1;
}

int sque_read_localized(const char *filepath, const char *key,
                        const char *lang, char *value_buf, int bufsize)
{
    FILE *fp;
    char line[MAX_LINE];
    char lang_tag[MAX_LINE];
    int in_field = 0;
    int in_lang = 0;
    int pos = 0;

    value_buf[0] = '\0';

    fp = fopen(filepath, "r");
    if (!fp) return -1;

    snprintf(lang_tag, sizeof(lang_tag), "[%s]", lang);

    while (sque_fgets(line, sizeof(line), fp) != NULL) {

        if (!in_field) {
            if (match_key(line, key)) {
                in_field = 1;
            }
            continue;
        }

        if (in_lang) {
            if (strcmp(line, "::end") == 0) break;
            if (is_section_line(line)) {
                if (match_key(line, lang)) { pos = 0; continue; }
                break;
            }
            if (pos + (int)strlen(line) + 1 < bufsize) {
                if (pos > 0) { value_buf[pos++] = '\n'; }
                strcpy(value_buf + pos, line);
                pos += (int)strlen(line);
            }
            continue;
        }

        if (strcmp(line, lang_tag) == 0) {
            in_lang = 1;
            continue;
        }

        if (is_section_line(line) && !match_key(line, lang)) {
            break;
        }
    }

    fclose(fp);

    if (in_lang && pos > 0) {
        value_buf[pos] = '\0';
        sque_utf8_to_acp(value_buf, bufsize);
        return pos;
    }
    return -1;
}

int sque_enum_localized(const char *filepath, const char *key,
                        sque_localized_cb cb, void *user)
{
    FILE *fp;
    char line[MAX_LINE];
    int in_field = 0;
    int count = 0;
    char current_lang[MAX_LINE];
    char current_value[MAX_LINE];
    int has_lang = 0;
    int pos = 0;

    fp = fopen(filepath, "r");
    if (!fp) return 0;

    current_lang[0] = '\0';
    current_value[0] = '\0';

    while (sque_fgets(line, sizeof(line), fp) != NULL) {

        if (!in_field) {
            if (match_key(line, key)) {
                in_field = 1;
            }
            continue;
        }

        if (is_section_line(line) && strcmp(line, "::end") != 0) {
            if (has_lang && pos > 0) {
                current_value[pos] = '\0';
                sque_utf8_to_acp(current_value, (int)sizeof(current_value));
                if (cb(current_lang, current_value, user) != 0) {
                    fclose(fp);
                    return count + 1;
                }
                count++;
            }

            if (!match_key(line, key)) {
                strncpy(current_lang, line + 1, sizeof(current_lang) - 1);
                current_lang[sizeof(current_lang) - 1] = '\0';
                {
                    char *end = strchr(current_lang, ']');
                    if (end) *end = '\0';
                }
                has_lang = 1;
                pos = 0;
                current_value[0] = '\0';
            } else {
                break;
            }
            continue;
        }

        if (strcmp(line, "::end") == 0) {
            if (has_lang && pos > 0) {
                current_value[pos] = '\0';
                sque_utf8_to_acp(current_value, (int)sizeof(current_value));
                if (cb(current_lang, current_value, user) != 0) {
                    fclose(fp);
                    return count + 1;
                }
                count++;
            }
            has_lang = 0;
            pos = 0;
            continue;
        }

        if (has_lang && strlen(line) > 0) {
            if (pos > 0 && pos + (int)strlen(line) + 1 < (int)sizeof(current_value)) {
                current_value[pos++] = '\n';
            }
            strncpy(current_value + pos, line, sizeof(current_value) - pos - 1);
            current_value[sizeof(current_value) - 1] = '\0';
            pos += (int)strlen(line);
        }
    }

    if (has_lang && pos > 0) {
        current_value[pos] = '\0';
        sque_utf8_to_acp(current_value, (int)sizeof(current_value));
        cb(current_lang, current_value, user);
        count++;
    }

    fclose(fp);
    return count;
}