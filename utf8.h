#ifndef TINYRL_UTF8_H
#define TINYRL_UTF8_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef DISABLE_UTF8
size_t utf8_char_len(char c);
size_t utf8_char_decode(const char *s, size_t len, uint32_t *dst);
size_t utf8_char_encode(uint32_t c, char *s, size_t len);
size_t utf8_char_next(const char *s, size_t len, size_t point);
size_t utf8_char_prev(const char *s, size_t len, size_t point);
size_t utf8_char_width(const char *s, size_t len, size_t point);
size_t utf8_grapheme_next(const char *s, size_t len, size_t point);
size_t utf8_grapheme_prev(const char *s, size_t len, size_t point);
size_t utf8_grapheme_width(const char *s, size_t len, size_t point, size_t *pnext);
#else
static inline size_t utf8_char_len(char c) {
	return 1;
}

static inline size_t utf8_char_decode(const char *s, size_t len, uint32_t *dst) {
	if (dst)
		*dst = *s;
	return 1;
}

static inline size_t utf8_char_encode(uint32_t c, char *s, size_t len) {
	if (len < 1)
		return 0;
	*s = c;
	return 1;
}

static inline size_t utf8_char_next(const char *s, size_t len, size_t point) {
	if (point >= len)
		return len;
	point++;
	return point;
}

static inline size_t utf8_char_prev(const char *s, size_t len, size_t point) {
	if (point <= 0)
		return 0;
	point--;
	return point;
}

static inline size_t utf8_char_width(const char *s, size_t len, size_t point) {
	if (s[point] < 0x20)
		return 0;
	if (s[point] < 0x7f)
		return 1;
	return 0;
}

static inline size_t utf8_grapheme_next(const char *s, size_t len, size_t point) {
	return utf8_char_next(s, len, point);
}

static inline size_t utf8_grapheme_prev(const char *s, size_t len, size_t point) {
	return utf8_char_prev(s, len, point);
}

static inline size_t utf8_grapheme_width(const char *s, size_t len, size_t point, size_t *pnext) {
	if (pnext)
		*pnext = utf8_char_next(s, len, point);
	return utf8_char_width(s, len, point);
}
#endif

#endif
