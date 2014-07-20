#include "complete.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/*-------------------------------------------------------- */
char **tinyrl_add_match(char **matches, const char *match)
{
	size_t len;
	char **m;

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
static void tinyrl_display_matches(const tinyrl_t * this, char *const *matches)
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
tinyrl_match_e tinyrl_complete(
	tinyrl_t * this, bool with_extensions, unsigned start,
	char **matches)
{
	tinyrl_match_e result = TINYRL_NO_MATCH;
	char *common;
	const char *line;
	unsigned end, len;
	bool completion = false;
	bool prefix = false;
	int i;

	line = tinyrl__get_line(this);
	end = tinyrl__get_point(this);
	if (matches) {
		/* identify and insert a common prefix if there is one */
		common = strdup(matches[0]);
		for (i = 1; matches[i]; i++) {
			char *p = common;
			char *q = matches[i];
			while (*p && tolower(*p) == tolower(*q)) {
				p++;
				q++;
			}
			*p = '\0';
		}
		len = strlen(common);
		if (end - start < len
		    || strncmp(line + start, common, len) != 0) {
			/* 
			 * delete the original text not including 
			 * the current insertion point character 
			 */
			if (end > start) {
				tinyrl_delete_text(this, start, end);
			}
			if (!tinyrl_insert_text(this, common)) {
				free(common);
				return TINYRL_NO_MATCH;
			}
			completion = true;
		}
		for (i = 0; matches[i]; i++) {
			if (strcmp(common, matches[i]) == 0) {
				/* this is just a prefix string */
				prefix = true;
			}
		}
		free(common);
		/* is there more than one completion? */
		if (matches[1] != NULL) {
			if (completion) {
				result = TINYRL_COMPLETED_AMBIGUOUS;
			} else if (prefix) {
				result = TINYRL_MATCH_WITH_EXTENSIONS;
			} else {
				result = TINYRL_AMBIGUOUS;
			}
			if (with_extensions || !prefix) {
				/* Either we always want to show extensions or
				 * we haven't been able to complete the current line
				 * and there is just a prefix, so let the user see the options
				 */
				tinyrl_crlf(this);
				tinyrl_display_matches(this, matches);
				tinyrl_reset_line_state(this);
			}
		} else {
			result =
				completion ? TINYRL_COMPLETED_MATCH : TINYRL_MATCH;
		}
		/* free the memory */
		tinyrl_delete_matches(matches);

		/* redisplay the line */
		tinyrl_redisplay(this);
	}
	return result;
}
