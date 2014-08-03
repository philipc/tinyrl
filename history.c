/*
 * history.c
 * 
 * Simple non-readline hooks for the cli library
 */
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <stdlib.h>

#include "history.h"

struct tinyrl_history {
	char **entries;	/* pointer entries */
	unsigned length;	/* Number of elements within this array */
	unsigned size;		/* Number of slots allocated in this array */
	unsigned limit;
};

/*------------------------------------- */
struct tinyrl_history *tinyrl_history_new(unsigned limit)
{
	struct tinyrl_history *history;
       
	history = malloc(sizeof(*history));
	if (!history)
		return NULL;

	history->entries = NULL;
	history->limit = limit;
	history->length = 0;
	history->size = 0;
	return history;
}

/*------------------------------------- */
void tinyrl_history_delete(struct tinyrl_history *history)
{
	unsigned i;

	for (i = 0; i < history->length; i++)
		free(history->entries[i]);
	free(history->entries);
	free(history);
}

/*
   HISTORY LIST MANAGEMENT 
   */
/*------------------------------------- */
/*
 * This removes the specified entries from the 
 * entries vector. Shuffling up the array as necessary 
 */
static void
remove_entries(struct tinyrl_history *history, unsigned start, unsigned delta)
{
	unsigned end = start + delta;
	unsigned i;

	assert(end <= history->length);

	for (i = start; i < end; i++)
		free(history->entries[i]);
	memmove(history->entries + start, history->entries + end,
		sizeof(*history->entries) * (history->length - end));
	history->length -= delta;
}

/*------------------------------------- */
/* 
   add an entry to the end of the current array 
   if there is no space returns -1 else 0
   */
static void append_entry(struct tinyrl_history *history, const char *line)
{
	if (history->length < history->size) {
		history->entries[history->length] = strdup(line);
		history->length++;
	}
}

/*------------------------------------- */
/* grow the array if necessary */
static void grow(struct tinyrl_history *history)
{
	if (history->size == history->length) {
		/* increment the history memory by 10 entries each time we grow */
		unsigned new_size = history->size + 10;
		char **new_entries;

		new_entries = realloc(history->entries,
				      sizeof(*history->entries) * new_size);
		if (NULL != new_entries) {
			history->size = new_size;
			history->entries = new_entries;
		}
	}
}

/*------------------------------------- */
void tinyrl_history_add(struct tinyrl_history *history, const char *line)
{
	if (history->length && (history->length == history->limit)) {
		/* remove the oldest entry */
		remove_entries(history, 0, 1);
	} else {
		grow(history);
	}
	append_entry(history, line);
}

/*------------------------------------- */
void tinyrl_history_remove(struct tinyrl_history *history, unsigned offset)
{
	if (offset < history->length) {
		/* do the biz */
		remove_entries(history, offset, 1);
	}
}

/*------------------------------------- */
void tinyrl_history_clear(struct tinyrl_history *history)
{
	/* free all the entries */
	remove_entries(history, 0, history->length);
}

/*------------------------------------- */
void tinyrl_history_limit(struct tinyrl_history *history, unsigned limit)
{
	if (limit && limit < history->length)
		remove_entries(history, 0, history->length - limit);
	history->limit = limit;
}

/*
   INFORMATION ABOUT THE HISTORY LIST 
   */
const char *tinyrl_history_get(const struct tinyrl_history *history,
			       unsigned position)
{
	if (position < history->length)
		return history->entries[position];
	return NULL;
}

size_t tinyrl_history_length(const struct tinyrl_history *history)
{
	return history->length;
}
