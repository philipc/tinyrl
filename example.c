#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "tinyrl.h"
#include "complete.h"

static char **add_match(char **matches, const char *match,
			const char *text, unsigned len)
{
	if (!len || strncmp(match, text, len) == 0)
		matches = tinyrl_add_match(matches, match);
	return matches;
}

static bool complete_key(tinyrl_t * t, int key)
{
	const char *text;
	unsigned start;
	unsigned len;
	char **matches;
	bool ret = false;

	/* find the start of the current word */
	text = tinyrl__get_line(t);
	start = len = tinyrl__get_point(t);
	while (start && !isspace(text[start - 1])) {
		start--;
	}
	text += start;
	len -= start;

	/* build a list of possible completions */
	matches = NULL;
	matches = add_match(matches, "exit", text, len);
	matches = add_match(matches, "help", text, len);
	matches = add_match(matches, "hello", text, len);
	if (!matches)
		return false;

	/* select the longest completion */
	if (tinyrl_complete(t, start, matches, false))
		ret = tinyrl_insert_text(t, " ");

	tinyrl_delete_matches(matches);

	return ret;
}

int main(int argc, char *argv[])
{
	tinyrl_t *t;
	char *line;

	t = tinyrl_new(stdin, stdout, 0);
	tinyrl_bind_key(t, '\t', complete_key);

	for (;;) {
		line = tinyrl_readline(t, "> ", NULL);
		if (!line)
			break;

		if (strcmp(line, "exit") == 0) {
			free(line);
			break;
		}

		printf("echo: %s\n", line);

		tinyrl_history_add(tinyrl__get_history(t), line);
		free(line);
	}

	tinyrl_delete(t);
	return 0;
}

