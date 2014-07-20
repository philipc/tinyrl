#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "tinyrl.h"
#include "complete.h"

static char **complete(tinyrl_t * t, const char *text,
		       unsigned start, unsigned end)
{
	char **matches = NULL;;

	if (start == end || text[start] == 'h') {
		matches = tinyrl_add_match(matches, "help");
		matches = tinyrl_add_match(matches, "hello");
	}

	return matches;
}

static bool complete_key(tinyrl_t * t, int key)
{
	tinyrl_match_e status;
	const char *line;
	unsigned start;
	unsigned end;
	char **matches;

	/* find the start of the current word */
	line = tinyrl__get_line(t);
	start = end = tinyrl__get_point(t);
	while (start && !isspace(line[start - 1])) {
		start--;
	}

	/* try and complete the word */
	matches = complete(t, line, start, end);

	status = tinyrl_complete(t, true, start, matches);
	switch (status) {
	case TINYRL_COMPLETED_MATCH:
	case TINYRL_MATCH:
		return tinyrl_insert_text(t, " ");
	case TINYRL_NO_MATCH:
	case TINYRL_MATCH_WITH_EXTENSIONS:
	case TINYRL_AMBIGUOUS:
	case TINYRL_COMPLETED_AMBIGUOUS:
		break;
	}

	/* let the bell ring */
	return false;
}

int main(int argc, char *argv[])
{
	tinyrl_t *t;
	char *line;

	t = tinyrl_new(stdin, stdout, 0);
	tinyrl_bind_key(t, '\t', complete_key);

	for (;;) {
		line = tinyrl_readline(t, "> ", NULL);
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

