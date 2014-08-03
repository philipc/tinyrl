#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "tinyrl.h"
#include "complete.h"

static bool complete(tinyrl_t *t, bool allow_prefix, bool allow_empty)
{
	const char *text;
	unsigned start;
	unsigned end;
	char **matches;
	bool ret = false;

	/* find the start of the current word */
	text = tinyrl__get_line(t);
	start = end = tinyrl__get_point(t);
	while (start && !isspace(text[start - 1]))
		start--;
	if (start == end && allow_empty)
		return true;

	/* build a list of possible completions */
	matches = NULL;
	matches = tinyrl_add_match(t, start, matches, "exit");
	matches = tinyrl_add_match(t, start, matches, "help");
	matches = tinyrl_add_match(t, start, matches, "hello");
	matches = tinyrl_add_match(t, start, matches, "vi");
	matches = tinyrl_add_match(t, start, matches, "view");
	if (!matches)
		return false;

	/* select the longest completion */
	ret = tinyrl_complete(t, start, matches, allow_prefix);

	tinyrl_delete_matches(matches);

	return ret;
}

static bool tab_key(tinyrl_t *t, int key)
{
	if (complete(t, false, false))
		return tinyrl_insert_text(t, " ");
	return false;
}

static bool space_key(tinyrl_t *t, int key)
{
	if (complete(t, true, false))
		return tinyrl_insert_text(t, " ");
	return false;
}

static bool enter_key(tinyrl_t *t, int key)
{
	if (complete(t, true, true)) {
		tinyrl_crlf(t);
		tinyrl_done(t);
	}
	return false;
}

int main(int argc, char *argv[])
{
	struct tinyrl_history *history;
	tinyrl_t *t;
	char *line;

	history = tinyrl_history_new(0);
	t = tinyrl_new(stdin, stdout, history);
	tinyrl_bind_key(t, '\t', tab_key);
	tinyrl_bind_key(t, '\r', enter_key);
	tinyrl_bind_key(t, ' ', space_key);

	for (;;) {
		line = tinyrl_readline(t, "> ", NULL);
		if (!line)
			break;

		if (strcmp(line, "exit") == 0) {
			free(line);
			break;
		}

		printf("echo: %s\n", line);

		tinyrl_history_add(history, line);
		free(line);
	}

	tinyrl_delete(t);
	tinyrl_history_delete(history);
	return 0;
}

