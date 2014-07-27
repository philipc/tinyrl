#ifndef _tinyrl_complete_h
#define _tinyrl_complete_h

#include "tinyrl.h"

char **tinyrl_add_match(
	const tinyrl_t *this, unsigned start, char **matches, const char *match);
void tinyrl_delete_matches(char **matches);
void tinyrl_display_matches(const tinyrl_t * this, char *const *matches);

/**
 * Complete the current word in the input buffer.
 *
 * The longest common prefix is inserted into the buffer.
 *
 * If there was only one possible match, then the result is true.
 *
 * If allow_prefix is true and the longest common prefix is also a valid match,
 * then the result is true.
 *
 * Otherwise the result is false.  In this case, if nothing was inserted into
 * the buffer then the matches are displayed.
 */
bool tinyrl_complete(
	tinyrl_t *this, unsigned start, char **matches, bool allow_prefix);

#endif
