/**
  \ingroup tinyrl
  \defgroup tinyrl_class tinyrl
  @{

  \brief This class provides instances which are capable of handling user input
  from a CLI in a "readline" like fashion.

*/
#ifndef TINYRL_TINYRL_H
#define TINYRL_TINYRL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct tinyrl;

enum tinyrl_key {
	TINYRL_KEY_UP,
	TINYRL_KEY_DOWN,
	TINYRL_KEY_LEFT,
	TINYRL_KEY_RIGHT,
	TINYRL_KEY_HOME,
	TINYRL_KEY_END,
	TINYRL_KEY_INSERT,
	TINYRL_KEY_DELETE,
};

/**
 * \return
 * - true if the action associated with the key has
 *   been performed successfully
 * - false if the action was not successful
 */
typedef bool tinyrl_key_func_t(void *context, char *key);

/* exported functions */
struct tinyrl *tinyrl_new(FILE * instream, FILE * outstream);

/*lint -esym(534,tinyrl_printf)  Ignoring return value of function */
int tinyrl_printf(struct tinyrl *instance, const char *fmt, ...);

void tinyrl_delete(struct tinyrl *instance);

void tinyrl_done(struct tinyrl *instance);

size_t tinyrl__get_width(const struct tinyrl *instance);

char *tinyrl_readline(struct tinyrl *instance, const char *prompt);

void tinyrl_bind_key(struct tinyrl *instance, unsigned char key,
		     tinyrl_key_func_t *handler, void *context);
void tinyrl_bind_special(struct tinyrl *instance, enum tinyrl_key key,
			 tinyrl_key_func_t *handler, void *context);

void tinyrl_crlf(struct tinyrl *instance);
void tinyrl_ding(struct tinyrl *instance);

void tinyrl_reset_line_state(struct tinyrl *instance);

bool tinyrl_insert_text(struct tinyrl *instance, const char *text);
bool tinyrl_insert_text_len(struct tinyrl *instance,
			    const char *text, unsigned len);
void tinyrl_delete_text( struct tinyrl *instance, unsigned start, unsigned end);

void tinyrl_redisplay(struct tinyrl *instance);

/* text must be persistent */
void tinyrl_set_line(struct tinyrl *instance, const char *text);

void tinyrl_replace_line(struct tinyrl *instance, const char *text);

/**
 * This operation returns the current line in use by the tinyrl instance
 * NB. the pointer will become invalid after any further operation on the 
 * instance.
 */
const char *tinyrl_get_line(const struct tinyrl *instance);

unsigned tinyrl_get_point(const struct tinyrl *instance);

/**
 * Disable echoing of input characters when a line in input.
 * 
 * echo_char is the character to display instead of a key press.
 * If this has the special value '/0' then the insertion point will not 
 * be moved when keys are pressed.
 */
void tinyrl_disable_echo(struct tinyrl *instance, char echo_char);

/**
 * Enable key echoing for this instance. (This is the default behaviour)
 */
void tinyrl_enable_echo(struct tinyrl *instance);

/**
 * Limit maximum line length
 *
 * 0 is unlimited
 */
void tinyrl_limit_line_length(struct tinyrl *instance, unsigned length);

#endif
/** @} tinyrl_tinyrl */
