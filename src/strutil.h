/*
 * keyd - A key remapping daemon.
 *
 * © 2019 Raheman Vaiya (see also: LICENSE).
 */
#ifndef STRUTIL_H
#define STRUTIL_H

#include <stdint.h>
#include <stdlib.h>
#include <string_view>

int utf8_read_char(const char *_s, uint32_t *code);
int utf8_read_char(std::string_view s, uint32_t& code);
int utf8_strlen(std::string_view s);

size_t str_escape(char *s);
#endif
