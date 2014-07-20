#include "complete.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/*-------------------------------------------------------- */
/*
 * Returns an array of strings which is a list of completions for text.
 * If there are no completions, returns NULL. The first entry in the
 * returned array is the substitution for text. The remaining entries
 * are the possible completions. The array is terminated with a NULL pointer.
 *
 * entry_func is a function of two args, and returns a char *. 
 * The first argument is text. The second is a state argument;
 * it is zero on the first call, and non-zero on subsequent calls.
 * entry_func returns a NULL pointer to the caller when there are no 
 * more matches. 
 */
char **tinyrl_completion(tinyrl_t * this,
			 const char *line,
			 unsigned start,
			 unsigned end, tinyrl_compentry_func_t * entry_func)
{
	unsigned state = 0;
	size_t size = 1;
	unsigned offset = 1;	/* need at least one entry for the substitution */
	char **matches = NULL;
	char *match;
	/* duplicate the string upto the insertion point */
	char *text = strndup(line, end);

	/* now try and find possible completions */
	while ((match = entry_func(this, text, start, state++))) {
		if (size == offset) {
			/* resize the buffer if needed - the +1 is for the NULL terminator */
			size += 10;
			matches =
				realloc(matches, (sizeof(char *) * (size + 1)));
		}
		if (NULL == matches) {
			/* not much we can do... */
			break;
		}
		matches[offset] = match;

		/*
		 * augment the substitute string with this entry
		 */
		if (1 == offset) {
			/* let's be optimistic */
			matches[0] = strdup(match);
		} else {
			char *p = matches[0];
			size_t match_len = strlen(p);
			/* identify the common prefix */
			while ((tolower(*p) == tolower(*match)) && match_len--) {
				p++, match++;
			}
			/* terminate the prefix string */
			*p = '\0';
		}
		offset++;
	}
	/* be a good memory citizen */
	free(text);

	if (matches) {
		matches[offset] = NULL;
	}
	return matches;
}

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
 * in argv format, such as a list of completion matches. len is the number
 * of strings in matches, and max is the length of the longest string in matches.
 * This function uses the setting of print-completions-horizontally to select
 * how the matches are displayed
 */
static void
tinyrl_display_matches(const tinyrl_t * this,
		       char *const *matches, unsigned len, size_t max)
{
	unsigned r, c;
	unsigned width = tinyrl__get_width(this);
	unsigned cols = width / (max + 1);	/* allow for a space between words */
	unsigned rows = len / cols + 1;

	assert(matches);
	if (matches) {
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
			char **tmp = matches;
			unsigned max, len;
			max = len = 0;
			while (*tmp) {
				size_t size = strlen(*tmp++);
				len++;
				if (size > max) {
					max = size;
				}
			}
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
				tinyrl_display_matches(this, matches, len, max);
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
