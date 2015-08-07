#include "utf8.h"

enum {
	UTF8_GRAPHEME_BREAK_OTHER,
	UTF8_GRAPHEME_BREAK_CR,
	UTF8_GRAPHEME_BREAK_LF,
	UTF8_GRAPHEME_BREAK_CONTROL,
	UTF8_GRAPHEME_BREAK_EXTEND,
	UTF8_GRAPHEME_BREAK_REGIONAL_INDICATOR,
	UTF8_GRAPHEME_BREAK_PREPEND,
	UTF8_GRAPHEME_BREAK_SPACINGMARK,
	UTF8_GRAPHEME_BREAK_L,
	UTF8_GRAPHEME_BREAK_V,
	UTF8_GRAPHEME_BREAK_T,
	UTF8_GRAPHEME_BREAK_LV,
	UTF8_GRAPHEME_BREAK_LVT,
};

struct utf8data {
	uint32_t lower;
	uint32_t upper;
	uint8_t width;
	uint8_t grapheme_break;
};

struct utf8data utf8data[] = {
#include "utf8data.c"
};

static bool utf8_cont(char c)
{
	return (c & 0xc0) == 0x80;
}

size_t utf8_char_len(char c)
{
	if ((c & 0x80) == 0x00)
		return 1;
	if ((c & 0xe0) == 0xc0)
		return 2;
	if ((c & 0xf0) == 0xe0)
		return 3;
	if ((c & 0xf8) == 0xf0)
		return 4;
	return 0;
}

size_t utf8_char_get(const char *s, size_t len, uint32_t *dst)
{
	size_t char_len;
	uint32_t c;

	char_len = utf8_char_len(*s);
	if (char_len > len)
		return 0;

	switch (char_len) {
	case 1:
		c = *s;
		break;
	case 2:
		if (!utf8_cont(s[1]))
			return 0;
		c = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
		if (c < 0x80)
			return 0;
		break;
	case 3:
		if (!utf8_cont(s[1]) || !utf8_cont(s[2]))
			return 0;
		c = ((s[0] & 0xf) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
		if (c < 0x800)
			return 0;
		if (c >= 0xd800 && c < 0xe000)
			return 0;
		break;
	case 4:
		if (!utf8_cont(s[1]) || !utf8_cont(s[2]) || !utf8_cont(s[3]))
			return 0;
		c = ((s[0] & 0x7) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
		if (c < 0x10000 || c >= 0x110000)
			return 0;
		break;
	default:
		return 0;
	}

	if (dst)
		*dst = c;
	return char_len;
}
