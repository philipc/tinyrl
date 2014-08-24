#include "complete.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "tinyrl.h"

/*-------------------------------------------------------- */
char **tinyrl_add_match(const struct tinyrl *this, unsigned start,
			char **matches, const char *match)
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
	if (matches)
		for (m = matches; *m; m++)
			len++;

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

	for (m = matches; *m; m++)
		free(*m);
	free(matches);
}

/*----------------------------------------------------------------------- */
/* 
 * A convenience function for displaying a list of strings in columnar
 * format on Readline's output stream. matches is the list of strings,
 * in argv format, such as a list of completion matches.
 */
void tinyrl_display_matches(const struct tinyrl *this, char *const *matches)
{
	char *const *m;
	size_t max;
	size_t c, cols;

	/* find maximum completion length */
	max = 0;
	for (m = matches; *m; m++) {
		size_t size = strlen(*m);
		if (max < size)
			max = size;
	}

	/* allow for a space between words */
	cols = tinyrl__get_width(this) / (max + 1);

	/* print out a table of completions */
	m = matches;
	for (m = matches; *m; ) {
		for (c = 0; c < cols && *m; c++, m++)
			tinyrl_printf(this, "%-*s ", max, *m);
		tinyrl_crlf(this);
	}
}

/*-------------------------------------------------------- */
bool tinyrl_complete(struct tinyrl *this, unsigned start,
		     char **matches, bool allow_prefix)
{
	const char *line;
	unsigned end, len;
	bool completion;
	bool prefix;
	int i;

	if (!matches || !matches[0])
		return false;

	/* identify common prefix */
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

	/* insert common prefix */
	line = tinyrl__get_line(this);
	end = tinyrl__get_point(this);
	if (end - start < len
	    || strncmp(line + start, matches[0], len) != 0) {
		tinyrl_delete_text(this, start, end);
		if (!tinyrl_insert_text_len(this, matches[0], len))
			return false;
		tinyrl_redisplay(this);
		completion = true;
	} else {
		completion = false;
	}

	/* is there only one completion? */
	if (!matches[1])
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
