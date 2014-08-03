/**
  \ingroup tinyrl
  \defgroup tinyrl_class tinyrl
  @{

  \brief This class provides instances which are capable of handling user input
  from a CLI in a "readline" like fashion.

*/
#ifndef _tinyrl_tinyrl_h
#define _tinyrl_tinyrl_h

#include <stdbool.h>
#include <stdio.h>
#include "history.h"

typedef struct _tinyrl tinyrl_t;

/* virtual methods */
typedef int tinyrl_hook_func_t(tinyrl_t * instance);

/**
 * \return
 * - true if the action associated with the key has
 *   been performed successfully
 * - false if the action was not successful
 */
typedef bool tinyrl_key_func_t(tinyrl_t * instance, int key);

/* exported functions */
extern tinyrl_t *tinyrl_new(FILE * instream,
			    FILE * outstream,
			    unsigned stifle);

/*lint -esym(534,tinyrl_printf)  Ignoring return value of function */
extern int tinyrl_printf(const tinyrl_t * instance, const char *fmt, ...);

extern void tinyrl_delete(tinyrl_t * instance);

extern struct tinyrl_history *tinyrl__get_history(const tinyrl_t * instance);

extern const char *tinyrl__get_prompt(const tinyrl_t * instance);

extern void tinyrl_done(tinyrl_t * instance);

extern void *tinyrl__get_context(const tinyrl_t * instance);

/**
 * This operation returns the current line in use by the tinyrl instance
 * NB. the pointer will become invalid after any further operation on the 
 * instance.
 */
extern const char *tinyrl__get_line(const tinyrl_t * instance);

extern unsigned tinyrl__get_point(const tinyrl_t * instance);

extern unsigned tinyrl__get_end(const tinyrl_t * instance);

extern unsigned tinyrl__get_width(const tinyrl_t * instance);

extern void tinyrl__set_istream(tinyrl_t * instance, FILE * istream);

extern bool tinyrl__get_isatty(const tinyrl_t * instance);

extern FILE *tinyrl__get_istream(const tinyrl_t * instance);

extern FILE *tinyrl__get_ostream(const tinyrl_t * instance);

extern char *tinyrl_readline(tinyrl_t * instance,
			     const char *prompt, void *context);
extern void
tinyrl_bind_key(tinyrl_t * instance, unsigned char key, tinyrl_key_func_t * fn);
extern void tinyrl_crlf(const tinyrl_t * instance);
extern void tinyrl_ding(const tinyrl_t * instance);

extern void tinyrl_reset_line_state(tinyrl_t * instance);

extern bool tinyrl_insert_text(
	tinyrl_t * instance, const char *text);
extern bool tinyrl_insert_text_len(
	tinyrl_t * instance, const char *text, unsigned len);
extern void tinyrl_delete_text(
	tinyrl_t * instance, unsigned start, unsigned end);

extern void tinyrl_redisplay(tinyrl_t * instance);

extern void
tinyrl_replace_line(tinyrl_t * instance, const char *text, int clear_undo);

/**
 * Disable echoing of input characters when a line in input.
 * 
 */
extern void tinyrl_disable_echo(
	/** 
	 * The instance on which to operate
	 */
	tinyrl_t * instance,
	/**
	 * The character to display instead of a key press.
	 *
	 * If this has the special value '/0' then the insertion point will not 
	 * be moved when keys are pressed.
	 */
	char echo_char);
/**
 * Enable key echoing for this instance. (This is the default behaviour)
 */
extern void tinyrl_enable_echo(
	/** 
	 * The instance on which to operate
	 */
	tinyrl_t * instance);
/**
 * Limit maximum line length
 */
extern void tinyrl_limit_line_length(
	/** 
	 * The instance on which to operate
	 */
	tinyrl_t * instance,
	/** 
	 * The length to limit to (0) is unlimited
	 */
	unsigned length);

#endif				/* _tinyrl_tinyrl_h */
/** @} tinyrl_tinyrl */
