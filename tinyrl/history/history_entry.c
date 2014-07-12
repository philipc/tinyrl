/* tinyrl_history_entry.c */
#include "private.h"
#include <stdlib.h>
#include <string.h>

struct _tinyrl_history_entry {
	char *line;
	unsigned index;
};
/*------------------------------------- */
static void
entry_init(tinyrl_history_entry_t * this, const char *line, unsigned index)
{
	this->line = strdup(line);
	this->index = index;
}

/*------------------------------------- */
static void entry_fini(tinyrl_history_entry_t * this)
{
	free(this->line);
	this->line = NULL;
}

/*------------------------------------- */
tinyrl_history_entry_t *tinyrl_history_entry_new(const char *line,
						 unsigned index)
{
	tinyrl_history_entry_t *this = malloc(sizeof(tinyrl_history_entry_t));
	if (NULL != this) {
		entry_init(this, line, index);
	}
	return this;
}

/*------------------------------------- */
void tinyrl_history_entry_delete(tinyrl_history_entry_t * this)
{
	entry_fini(this);
	free(this);
}

/*------------------------------------- */
const char *tinyrl_history_entry__get_line(const tinyrl_history_entry_t * this)
{
	return this->line;
}

/*------------------------------------- */
unsigned tinyrl_history_entry__get_index(const tinyrl_history_entry_t * this)
{
	return this->index;
}

/*------------------------------------- */
