#ifndef TINYRL_UTF8_H
#define TINYRL_UTF8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

size_t utf8_char_len(char c);
size_t utf8_char_get(const char *s, size_t len, uint32_t *dst);

#endif
