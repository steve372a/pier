#ifndef SQUE_H
#define SQUE_H

typedef int (*sque_localized_cb)(const char *lang, const char *value, void *user);

/* Read all content under [key] section, multi-line joined with \n.
 * Stops at next [section], ::end, or EOF.
 * Automatically converts UTF-8 to system ACP.
 * Returns length of value, or -1 if not found. */
int sque_read(const char *filepath, const char *key,
              char *value_buf, int bufsize);

/* Read all content under [key] section for a specific language tag.
 * Within the [key] section, looks for [lang] and reads its content.
 * Multi-line content joined with \n, until ::end or next [ or EOF.
 * Automatically converts UTF-8 to system ACP.
 * Returns length of value, or -1 if not found. */
int sque_read_localized(const char *filepath, const char *key,
                        const char *lang, char *value_buf, int bufsize);

/* Enumerate all language tags under [key] section, calling cb for each.
 * Automatically converts UTF-8 to system ACP for each value.
 * Returns number of languages enumerated. */
int sque_enum_localized(const char *filepath, const char *key,
                        sque_localized_cb cb, void *user);

/* Convert UTF-8 string to system ACP (ANSI code page) in-place.
 * Does nothing if input is not valid UTF-8 (Windows XP+ safe). */
void sque_utf8_to_acp(char *str, int max_len);

#endif