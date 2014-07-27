#include "complete.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/*-------------------------------------------------------- */
char **tinyrl_add_match(
	const tinyrl_t *this, unsigned start, char **matches, const char *match)
{
	const char *line;
	unsigned end;
	size_t len;
	char **m;

	line = tinyrl__get_line(this);
	end = tinyrl__get_point(this);
	if (strncmp(match, line + start, end - start) != 0)
		return matches;

	len = 0;
	if (matches) {
		for (m = matches; *m; m++)
			len++;
	}

	/* Allocate memory for new match, plus terminator */
	matches = realloc(matches, (len + 2) * sizeof(*matches));
	matches[len] = strdup(match);
	matches[len+1] = NULL;
	return matches;
}

/*-------------------------------------------------------- */
void tinyrl_delete_matches(char **matches)
{
	char **m;
	for (m = matches; *m; m++) {
		/* release the memory for each contained string */
		free(*m);
	}
	/* release the memory for the array */
	free(matches);
}

/*----------------------------------------------------------------------- */
/* 
 * A convenience function for displaying a list of strings in columnar
 * format on Readline's output stream. matches is the list of strings,
 * in argv format, such as a list of completion matches.
 */
void tinyrl_display_matches(const tinyrl_t * this, char *const *matches)
{
	char *const *m;
	unsigned max, len;
	unsigned r, c;
	unsigned width;
	unsigned cols, rows;

	max = len = 0;
	for (m = matches; *m; m++) {
		size_t size = strlen(*m);
		if (size > max) {
			max = size;
		}
		len++;
	}

	width = tinyrl__get_width(this);
	cols = width / (max + 1);	/* allow for a space between words */
	rows = len / cols + 1;

	/* print out a table of completions */
	for (r = 0; r < rows && len; r++) {
		for (c = 0; c < cols && len; c++) {
			const char *match = *matches++;
			len--;
			tinyrl_printf(this, "%-*s ", max, match);
		}
		tinyrl_crlf(this);
	}
}

/*-------------------------------------------------------- */
bool tinyrl_complete(
	tinyrl_t *this, unsigned start, char **matches, bool allow_prefix)
{
	const char *line;
	unsigned end, len;
	bool completion = false;
	bool prefix;
	int i;

	if (!matches)
		return false;

	line = tinyrl__get_line(this);
	end = tinyrl__get_point(this);
	/* identify and insert a common prefix if there is one */
	len = strlen(matches[0]);
	prefix = true;
	for (i = 1; matches[i]; i++) {
		unsigned common;
		for (common = 0; common < len; common++)
			if (matches[0][common] != matches[i][common])
				break;
		if (len != common) {
			len = common;
			prefix = !matches[i][len];
		}
	}
	if (end - start < len
	    || strncmp(line + start, matches[0], len) != 0) {
		tinyrl_delete_text(this, start, end);
		if (!tinyrl_insert_text_len(this, matches[0], len))
			return false;
		tinyrl_redisplay(this);
		completion = true;
	}

	/* is there more than one completion? */
	if (matches[1] == NULL)
		return true;
	/* is the prefix valid? */
	if (prefix && allow_prefix)
		return true;
	/* display matches if no progress was made */
	if (!completion) {
		tinyrl_crlf(this);
		tinyrl_display_matches(this, matches);
		tinyrl_reset_line_state(this);
	}
	return false;
}
