#include <stdlib.h>
#include <string.h>
#include "tinyrl.h"
#include "complete.h"

static char **complete(tinyrl_t * t, const char *text,
		       unsigned start, unsigned end)
{
	char **ret;

	if (start == end || text[start] == 'h') {
		ret = malloc(4 * sizeof(*ret));
		ret[0] = strdup("hel");
		ret[1] = strdup("help");
		ret[2] = strdup("hello");
		ret[3] = NULL;
		return ret;
	}

	return NULL;
}

static bool complete_key(tinyrl_t * this, int key)
{
	return tinyrl_complete_key(this, complete);
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

