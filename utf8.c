#include "utf8.h"
#include <stdlib.h>

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

#include "utf8data.c"

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

size_t utf8_char_decode(const char *s, size_t len, uint32_t *dst)
{
	size_t char_len;
	uint32_t c;

	char_len = utf8_char_len(*s);
	if (char_len > len)
		goto err;

	switch (char_len) {
	case 1:
		c = *s;
		break;
	case 2:
		if (!utf8_cont(s[1]))
			goto err;
		c = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
		if (c < 0x80)
			goto err;
		break;
	case 3:
		if (!utf8_cont(s[1]) || !utf8_cont(s[2]))
			goto err;
		c = ((s[0] & 0xf) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
		if (c < 0x800)
			goto err;
		if (c >= 0xd800 && c < 0xe000)
			goto err;
		break;
	case 4:
		if (!utf8_cont(s[1]) || !utf8_cont(s[2]) || !utf8_cont(s[3]))
			goto err;
		c = ((s[0] & 0x7) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
		if (c < 0x10000 || c >= 0x110000)
			goto err;
		break;
	default:
		goto err;
	}

	if (dst)
		*dst = c;
	return char_len;

err:
	if (dst)
		*dst = 0;
	return 0;
}

size_t utf8_char_next(const char *s, size_t len, size_t point)
{
	for (;;) {
		point++;
		if (point >= len)
			return point;
		if (!utf8_cont(s[point]))
			return point;
	}
}

size_t utf8_char_prev(const char *s, size_t len, size_t point)
{
	for (;;) {
		if (point == 0)
			return point;
		point--;
		if (!utf8_cont(s[point]))
			return point;
	}
}

size_t utf8_char_width(const char *s, size_t len, size_t point)
{
	uint32_t c;
	uint8_t i;

	utf8_char_decode(s + point, len - point, &c);
	if (c >= 0x110000)
		return 0;
	i = width0[c >> width0_shift];
	i = width1[i][(c >> width1_shift) & width1_mask];
	i = width2[i][(c >> width2_shift) & width2_mask];
	return (i >> ((c & width3_mask) * width_val_shift)) & width_val_mask;
}

static int utf8_grapheme_boundary_class(uint32_t c)
{
	uint8_t i;

	i = grapheme_break0[c >> grapheme_break0_shift];
	i = grapheme_break1[i][(c >> grapheme_break1_shift) & grapheme_break1_mask];
	i = grapheme_break2[i][(c >> grapheme_break2_shift) & grapheme_break2_mask];
	return (i >> ((c & grapheme_break3_mask) * grapheme_break_val_shift)) & grapheme_break_val_mask;
}

static bool utf8_grapheme_break(uint32_t c1, uint32_t c2)
{
	int b1, b2;

	b1 = utf8_grapheme_boundary_class(c1);
	b2 = utf8_grapheme_boundary_class(c2);

	/* GB3 */
	if (b1 == UTF8_GRAPHEME_BREAK_CR && b2 == UTF8_GRAPHEME_BREAK_LF)
		return false;

	/* GB4 */
	if (b1 == UTF8_GRAPHEME_BREAK_CR || b1 == UTF8_GRAPHEME_BREAK_LF || b1 == UTF8_GRAPHEME_BREAK_CONTROL)
		return true;

	/* GB5 */
	if (b2 == UTF8_GRAPHEME_BREAK_CR || b2 == UTF8_GRAPHEME_BREAK_LF || b2 == UTF8_GRAPHEME_BREAK_CONTROL)
		return true;

	/* GB6 */
	if (b1 == UTF8_GRAPHEME_BREAK_L && (b2 == UTF8_GRAPHEME_BREAK_L || b2 == UTF8_GRAPHEME_BREAK_V || b2 == UTF8_GRAPHEME_BREAK_LV || b2 == UTF8_GRAPHEME_BREAK_LVT))
		return false;

	/* GB7 */
	if ((b1 == UTF8_GRAPHEME_BREAK_LV || b1 == UTF8_GRAPHEME_BREAK_V) && (b2 == UTF8_GRAPHEME_BREAK_V || b2 == UTF8_GRAPHEME_BREAK_T))
		return false;

	/* GB8 */
	if ((b1 == UTF8_GRAPHEME_BREAK_LVT || b1 == UTF8_GRAPHEME_BREAK_T) && b2 == UTF8_GRAPHEME_BREAK_T)
		return false;

	/* GB8a */
	if (b1 == UTF8_GRAPHEME_BREAK_REGIONAL_INDICATOR && b2 == UTF8_GRAPHEME_BREAK_REGIONAL_INDICATOR)
		return false;

	/* GB9 */
	if (b2 == UTF8_GRAPHEME_BREAK_EXTEND)
		return false;

	/* GB9a */
	if (b2 == UTF8_GRAPHEME_BREAK_SPACINGMARK)
		return false;

	/* GB9b */
	if (b1 == UTF8_GRAPHEME_BREAK_PREPEND)
		return false;

	/* GB10 */
	return true;
}

size_t utf8_grapheme_next(const char *s, size_t len, size_t point)
{
	uint32_t c1, c2;

	utf8_char_decode(s + point, len - point, &c1);
	for (;;) {
		point = utf8_char_next(s, len, point);
		if (point >= len)
			return point;
		utf8_char_decode(s + point, len - point, &c2);
		if (utf8_grapheme_break(c1, c2))
			return point;
		c1 = c2;
	}
}

size_t utf8_grapheme_prev(const char *s, size_t len, size_t point)
{
	uint32_t c1, c2;
	size_t prev;

	point = utf8_char_prev(s, len, point);
	utf8_char_decode(s + point, len - point, &c2);
	for (;;) {
		if (point == 0)
			return point;
		prev = utf8_char_prev(s, len, point);
		utf8_char_decode(s + prev, len - prev, &c1);
		if (utf8_grapheme_break(c1, c2))
			return point;
		point = prev;
		c2 = c1;
	}
}

size_t utf8_grapheme_width(const char *s, size_t len, size_t point, size_t *pnext)
{
	size_t next;
	size_t width;
	size_t i;

	next = utf8_grapheme_next(s, len, point);
	if (pnext) *pnext = next;

	width = 0;
	for (i = point; i < next; i = utf8_char_next(s, len, i))
		width += utf8_char_width(s, len, i);
	return width;
}
