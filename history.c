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

struct _tinyrl_history {
	char **entries;	/* pointer entries */
	unsigned length;	/* Number of elements within this array */
	unsigned size;		/* Number of slots allocated in this array */
	unsigned limit;
};

/*------------------------------------- */
tinyrl_history_t *tinyrl_history_new(unsigned limit)
{
	tinyrl_history_t *this;
       
	this = malloc(sizeof(*this));
	if (!this)
		return NULL;

	this->entries = NULL;
	this->limit = limit;
	this->length = 0;
	this->size = 0;
	return this;
}

/*------------------------------------- */
void tinyrl_history_delete(tinyrl_history_t * this)
{
	unsigned i;

	for (i = 0; i < this->length; i++)
		free(this->entries[i]);
	free(this->entries);
	free(this);
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
remove_entries(tinyrl_history_t * this, unsigned start, unsigned delta)
{
	unsigned end = start + delta;
	unsigned i;

	assert(end <= this->length);

	for (i = start; i < end; i++)
		free(this->entries[i]);
	memmove(this->entries + start, this->entries + end,
		sizeof(*this->entries) * (this->length - end));
	this->length -= delta;
}

/*------------------------------------- */
/* 
   add an entry to the end of the current array 
   if there is no space returns -1 else 0
   */
static void append_entry(tinyrl_history_t * this, const char *line)
{
	if (this->length < this->size) {
		this->entries[this->length] = strdup(line);
		this->length++;
	}
}

/*------------------------------------- */
/* grow the array if necessary */
static void grow(tinyrl_history_t * this)
{
	if (this->size == this->length) {
		/* increment the history memory by 10 entries each time we grow */
		unsigned new_size = this->size + 10;
		char **new_entries;

		new_entries = realloc(this->entries,
				      sizeof(*this->entries) * new_size);
		if (NULL != new_entries) {
			this->size = new_size;
			this->entries = new_entries;
		}
	}
}

/*------------------------------------- */
void tinyrl_history_add(tinyrl_history_t * this, const char *line)
{
	if (this->length && (this->length == this->limit)) {
		/* remove the oldest entry */
		remove_entries(this, 0, 1);
	} else {
		grow(this);
	}
	append_entry(this, line);
}

/*------------------------------------- */
void tinyrl_history_remove(tinyrl_history_t *this, unsigned offset)
{
	if (offset < this->length) {
		/* do the biz */
		remove_entries(this, offset, 1);
	}
}

/*------------------------------------- */
void tinyrl_history_clear(tinyrl_history_t * this)
{
	/* free all the entries */
	remove_entries(this, 0, this->length);
}

/*------------------------------------- */
void tinyrl_history_limit(tinyrl_history_t * this, unsigned limit)
{
	if (limit && limit < this->length)
		remove_entries(this, 0, this->length - limit);
	this->limit = limit;
}

/*
   INFORMATION ABOUT THE HISTORY LIST 
   */
const char *tinyrl_history_get(const tinyrl_history_t * this,
			       unsigned position)
{
	if (position < this->length)
		return this->entries[position];
	return NULL;
}

size_t tinyrl_history_length(const tinyrl_history_t * this)
{
	return this->length;
}
