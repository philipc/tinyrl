#ifndef _tinyrl_complete_h
#define _tinyrl_complete_h

#include "tinyrl.h"

typedef enum {
	/**
	 * no possible completions were found
	 */
	TINYRL_NO_MATCH = 0,
	/**
	 * the provided string was already an exact match
	 */
	TINYRL_MATCH,
	/**
	 * the provided string was ambiguous and produced
	 * more than one possible completion
	 */
	TINYRL_AMBIGUOUS,
	/**
	 * the provided string was unambiguous and a 
	 * completion was performed
	 */
	TINYRL_COMPLETED_MATCH,
	/**
	 * the provided string was ambiguous but a partial
	 * completion was performed.
	 */
	TINYRL_COMPLETED_AMBIGUOUS,
	/**
	 * the provided string was an exact match for one
	 * possible value but there are other exetensions
	 * of the string available.
	 */
	TINYRL_MATCH_WITH_EXTENSIONS
} tinyrl_match_e;

char **tinyrl_add_match(char **matches, const char *match);
void tinyrl_delete_matches(char **matches);
void tinyrl_display_matches(const tinyrl_t * this, char *const *matches);

/**
 * Complete the current word in the input buffer.
 *
 * \return
 * - the type of match performed.
 */
tinyrl_match_e tinyrl_complete(
	tinyrl_t * this, unsigned start, char **matches);

#endif
