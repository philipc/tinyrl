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

typedef char **tinyrl_completion_func_t(tinyrl_t * instance,
					const char *text,
					unsigned start, unsigned end);

extern void tinyrl_delete_matches(char **instance);
extern char **tinyrl_completion(tinyrl_t * instance,
				const char *line,
				unsigned start,
				unsigned end,
				tinyrl_compentry_func_t * generator);

/**
 * Complete the current word in the input buffer, displaying
 * a prompt to clarify any abiguity if necessary.
 *
 * \return
 * - the type of match performed.
 * \post
 * - If the current word is ambiguous then a list of 
 *   possible completions will be displayed.
 */
tinyrl_match_e tinyrl_complete(tinyrl_t * this, bool with_extensions,
			       tinyrl_completion_func_t *complete_fn);

/**
 * Complete the current word in the input buffer, displaying
 * a prompt to clarify any abiguity or extra extensions if necessary.
 *
 * These callback should be bound to the key that is used for
 * completion using tinyrl_bind_key().
 */
bool tinyrl_complete_key(tinyrl_t * this,
			 tinyrl_completion_func_t *complete_fn);

#endif
