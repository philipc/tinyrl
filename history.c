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
	unsigned stifle;
};

/*------------------------------------- */
tinyrl_history_t *tinyrl_history_new(unsigned stifle)
{
	tinyrl_history_t *this;
       
	this = malloc(sizeof(*this));
	if (!this)
		return NULL;

	this->entries = NULL;
	this->stifle = stifle;
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
/* insert a new entry at the current offset */
static void insert_entry(tinyrl_history_t * this, const char *line)
{
	char *new_entry = strdup(line);
	assert(this->length);
	assert(this->entries);
	if (new_entry) {
		this->entries[this->length - 1] = new_entry;
	}
}

/*------------------------------------- */
/*
 * This frees the specified entries from the 
 * entries vector. NB it doesn't perform any shuffling.
 * This function is inclusive of start and end
 */
static void
free_entries(const tinyrl_history_t * this, unsigned start, unsigned end)
{
	unsigned i;
	assert(start <= end);
	assert(end < this->length);

	for (i = start; i <= end; i++)
		free(this->entries[i]);
}

/*------------------------------------- */
/*
 * This removes the specified entries from the 
 * entries vector. Shuffling up the array as necessary 
 * This function is inclusive of start and end
 */
static void
remove_entries(tinyrl_history_t * this, unsigned start, unsigned end)
{
	unsigned delta = (end - start) + 1;	/* number of entries being deleted */
	/* number of entries to shuffle */
	unsigned num_entries = (this->length - end) - 1;
	assert(start <= end);
	assert(end < this->length);

	if (num_entries) {
		/* move the remaining entries down to close the array */
		memmove(&this->entries[start],
			&this->entries[end + 1],
			sizeof(*this->entries) * num_entries);
	}
	/* now fix up the length variables */
	this->length -= delta;
}

/*------------------------------------- */
/* 
   Search the current history buffer for the specified 
   line and if found remove it.
   */
static bool remove_duplicate(tinyrl_history_t * this, const char *line)
{
	bool result = false;
	unsigned i;

	for (i = 0; i < this->length; i++) {
		if (strcmp(line, this->entries[i]) == 0) {
			free_entries(this, i, i);
			remove_entries(this, i, i);
			result = true;
			break;
		}
	}
	return result;
}

/*------------------------------------- */
/* 
   add an entry to the end of the current array 
   if there is no space returns -1 else 0
   */
static void append_entry(tinyrl_history_t * this, const char *line)
{
	if (this->length < this->size) {
		this->length++;

		insert_entry(this, line);
	}
}

/*------------------------------------- */
/*
   add a new history entry replacing the oldest one 
   */
static void add_n_replace(tinyrl_history_t * this, const char *line)
{
	if (!remove_duplicate(this, line)) {
		/* free the oldest entry */
		free_entries(this, 0, 0);
		/* shuffle the array */
		remove_entries(this, 0, 0);
	}
	/* add the new entry */
	append_entry(this, line);
}

/*------------------------------------- */
/* add a new history entry growing the array if necessary */
static void add_n_grow(tinyrl_history_t * this, const char *line)
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
	(void)remove_duplicate(this, line);
	append_entry(this, line);
}

/*------------------------------------- */
void tinyrl_history_add(tinyrl_history_t * this, const char *line)
{
	if (this->length && (this->length == this->stifle)) {
		add_n_replace(this, line);
	} else {
		add_n_grow(this, line);
	}
}

/*------------------------------------- */
char *tinyrl_history_remove(tinyrl_history_t *this, unsigned offset)
{
	char *result = NULL;

	if (offset < this->length) {
		result = this->entries[offset];
		/* do the biz */
		remove_entries(this, offset, offset);
	}
	return result;
}

/*------------------------------------- */
void tinyrl_history_clear(tinyrl_history_t * this)
{
	/* free all the entries */
	free_entries(this, 0, this->length - 1);
	/* and shuffle the array */
	remove_entries(this, 0, this->length - 1);
}

/*------------------------------------- */
void tinyrl_history_stifle(tinyrl_history_t * this, unsigned stifle)
{
	/* 
	 * if we are stifling (i.e. non zero value) then 
	 * delete the obsolete entries
	 */
	if (stifle) {
		if (stifle < this->length) {
			unsigned num_deletes = this->length - stifle;
			/* free the entries */
			free_entries(this, 0, num_deletes - 1);
			/* shuffle the array shut */
			remove_entries(this, 0, num_deletes - 1);
		}
		this->stifle = stifle;
	}
}

/*------------------------------------- */
unsigned tinyrl_history_unstifle(tinyrl_history_t * this)
{
	unsigned result = this->stifle;

	this->stifle = 0;

	return result;
}

/*------------------------------------- */
bool tinyrl_history_is_stifled(const tinyrl_history_t * this)
{
	return this->stifle != 0;
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
