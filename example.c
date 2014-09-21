#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "tinyrl.h"
#include "history.h"
#include "complete.h"

static bool complete(struct tinyrl *t, bool allow_prefix, bool allow_empty)
{
	const char *text;
	unsigned start;
	unsigned end;
	char **matches;
	bool ret = false;

	/* find the start of the current word */
	text = tinyrl_get_line(t);
	start = end = tinyrl_get_point(t);
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

static bool tab_key(void *context, int key)
{
	struct tinyrl *t = context;

	if (complete(t, false, false))
		return tinyrl_insert_text(t, " ");
	return false;
}

static bool space_key(void *context, int key)
{
	struct tinyrl *t = context;

	if (complete(t, true, false))
		return tinyrl_insert_text(t, " ");
	return false;
}

static bool enter_key(void *context, int key)
{
	struct tinyrl *t = context;

	if (complete(t, true, true)) {
		tinyrl_crlf(t);
		tinyrl_done(t);
	}
	return false;
}

int main(int argc, char *argv[])
{
	struct tinyrl_history *history;
	struct tinyrl *t;
	char *line;

	t = tinyrl_new(stdin, stdout);
	tinyrl_bind_key(t, '\t', tab_key, t);
	tinyrl_bind_key(t, '\r', enter_key, t);
	tinyrl_bind_key(t, ' ', space_key, t);

	history = tinyrl_history_new(t, 0);

	for (;;) {
		line = tinyrl_readline(t, "> ");
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

	tinyrl_history_delete(history);
	tinyrl_delete(t);
	return 0;
}

