/*
 * tinyrl.c
 */

/* make sure we can get fileno() */
#undef __STRICT_ANSI__

/* LIBC HEADERS */
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* POSIX HEADERS */
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "tinyrl.h"

/**
 * This enumeration is used to identify the types of escape code 
 */
typedef enum {
	tinyrl_vt100_UNKNOWN,	   /**< Undefined escape sequence */
	tinyrl_vt100_CURSOR_UP,	   /**< Move the cursor up        */
	tinyrl_vt100_CURSOR_DOWN,  /**< Move the cursor down      */
	tinyrl_vt100_CURSOR_LEFT,  /**< Move the cursor left      */
	tinyrl_vt100_CURSOR_RIGHT  /**< Move the cursor right     */
} tinyrl_vt100_escape_t;

typedef struct {
	const char terminator;
	tinyrl_vt100_escape_t code;
} vt100_decode_t;

/* define the class member data and virtual methods */
struct _tinyrl {
	FILE *istream;
	FILE *ostream;
	const char *line;
	unsigned max_line_length;
	const char *prompt;
	char *buffer;
	size_t buffer_size;
	bool done;
	unsigned point;
	unsigned end;
	char *kill_string;
#define NUM_HANDLERS 256
	tinyrl_key_func_t *handlers[NUM_HANDLERS];

	tinyrl_history_t *history;
	tinyrl_history_iterator_t hist_iter;
	void *context;		/* context supplied by caller
				 * to tinyrl_readline()
				 */
	char echo_char;
	bool echo_enabled;
	struct termios default_termios;
	bool isatty;
	char *last_buffer;	/* hold record of the previous 
				   buffer for redisplay purposes */
	unsigned last_point;	/* hold record of the previous 
				   cursor position for redisplay purposes */
};

#define ESCAPE 27
#define BACKSPACE 127

/* This table maps the vt100 escape codes to an enumeration */
static vt100_decode_t cmds[] = {
	{'A', tinyrl_vt100_CURSOR_UP},
	{'B', tinyrl_vt100_CURSOR_DOWN},
	{'C', tinyrl_vt100_CURSOR_RIGHT},
	{'D', tinyrl_vt100_CURSOR_LEFT},
};

/*--------------------------------------------------------- */
static void _tinyrl_vt100_setInputNonBlocking(const tinyrl_t * this)
{
#if defined(STDIN_FILENO)
	int flags = (fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
	fcntl(STDIN_FILENO, F_SETFL, flags);
#endif				/* STDIN_FILENO */
}

/*--------------------------------------------------------- */
static void _tinyrl_vt100_setInputBlocking(const tinyrl_t * this)
{
#if defined(STDIN_FILENO)
	int flags = (fcntl(STDIN_FILENO, F_GETFL, 0) & ~O_NONBLOCK);
	fcntl(STDIN_FILENO, F_SETFL, flags);
#endif				/* STDIN_FILENO */
}

/*--------------------------------------------------------- */
static tinyrl_vt100_escape_t tinyrl_vt100_escape_decode(const tinyrl_t * this)
{
	tinyrl_vt100_escape_t result = tinyrl_vt100_UNKNOWN;
	char sequence[10], *p = sequence;
	int c;
	unsigned i;
	/* before the while loop, set the input as non-blocking */
	_tinyrl_vt100_setInputNonBlocking(this);

	/* dump the control sequence into our sequence buffer 
	 * ANSI standard control sequences will end 
	 * with a character between 64 - 126
	 */
	while (1) {
		c = getc(this->istream);

		/* ignore no-character condition */
		if (-1 != c) {
			*p++ = (c & 0xFF);
			if ((c != '[') && (c > 63)) {
				/* this is an ANSI control sequence terminator code */
				result = tinyrl_vt100_CURSOR_UP;	/* just a non-UNKNOWN value */
				break;
			}
		} else {
			result = tinyrl_vt100_UNKNOWN;
			break;
		}
	}
	/* terminate the string (for debug purposes) */
	*p = '\0';

	/* restore the blocking status */
	_tinyrl_vt100_setInputBlocking(this);

	if (tinyrl_vt100_UNKNOWN != result) {
		/* now decode the sequence */
		for (i = 0; i < sizeof(cmds) / sizeof(vt100_decode_t); i++) {
			if (cmds[i].terminator == c) {
				/* found the code in the lookup table */
				result = cmds[i].code;
				break;
			}
		}
	}

	return result;
}

/*-------------------------------------------------------- */
static void tinyrl_vt100_clear_screen(const tinyrl_t * this)
{
	tinyrl_printf(this, "\x1b[2J");
}

/*-------------------------------------------------------- */
static void tinyrl_vt100_cursor_forward(const tinyrl_t * this, unsigned count)
{
	tinyrl_printf(this, "\x1b[%dC", count);
}

/*-------------------------------------------------------- */
static void tinyrl_vt100_cursor_back(const tinyrl_t * this, unsigned count)
{
	tinyrl_printf(this, "\x1b[%dD", count);
}

/*-------------------------------------------------------- */
static void tinyrl_vt100_cursor_home(const tinyrl_t * this)
{
	tinyrl_printf(this, "\x1b[H");
}

/*-------------------------------------------------------- */
static void tinyrl_vt100_erase(const tinyrl_t * this, unsigned count)
{
	tinyrl_printf(this, "\x1b[%dP", count);
}

/*----------------------------------------------------------------------- */
static void tty_set_raw_mode(tinyrl_t * this)
{
	struct termios new_termios;
	int fd = fileno(this->istream);
	int status;

	status = tcgetattr(fd, &this->default_termios);
	if (-1 != status) {
		status = tcgetattr(fd, &new_termios);
		assert(-1 != status);
		new_termios.c_iflag = 0;
		new_termios.c_oflag = OPOST | ONLCR;
		new_termios.c_lflag = 0;
		new_termios.c_cc[VMIN] = 1;
		new_termios.c_cc[VTIME] = 0;
		/* Do the mode switch */
		status = tcsetattr(fd, TCSAFLUSH, &new_termios);
		assert(-1 != status);
	}
}

/*----------------------------------------------------------------------- */
static void tty_restore_mode(const tinyrl_t * this)
{
	int fd = fileno(this->istream);

	/* Do the mode switch */
	(void)tcsetattr(fd, TCSAFLUSH, &this->default_termios);
}

/*----------------------------------------------------------------------- */
/*
   This is called whenever a line is edited in any way.
   It signals that if we are currently viewing a history line we should transfer it
   to the current buffer
   */
static void changed_line(tinyrl_t * this)
{
	/* if the current line is not our buffer then make it so */
	if (this->line != this->buffer) {
		/* replace the current buffer with the new details */
		free(this->buffer);
		this->line = this->buffer = strdup(this->line);
		this->buffer_size = strlen(this->buffer);
		assert(this->line);
	}
}

/*----------------------------------------------------------------------- */
static bool tinyrl_key_default(tinyrl_t * this, int key)
{
	bool result;
	if (key > 31) {
		char tmp = key & 0xFF;
		result = tinyrl_insert_text_len(this, &tmp, 1);
	} else {
		char tmp[10];
		sprintf(tmp, "~%d", key);
		/* inject control characters as ~N where N is the ASCII code */
		result = tinyrl_insert_text(this, tmp);
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_interrupt(tinyrl_t * this, int key)
{
	tinyrl_delete_text(this, 0, this->end);
	this->done = true;

	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_start_of_line(tinyrl_t * this, int key)
{
	/* set the insertion point to the start of the line */
	this->point = 0;
	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_end_of_line(tinyrl_t * this, int key)
{
	/* set the insertion point to the end of the line */
	this->point = this->end;
	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_kill(tinyrl_t * this, int key)
{
	/* release any old kill string */
	free(this->kill_string);

	/* store the killed string */
	this->kill_string = strdup(&this->buffer[this->point]);

	/* delete the text to the end of the line */
	tinyrl_delete_text(this, this->point, this->end);
	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_yank(tinyrl_t * this, int key)
{
	bool result = false;
	if (this->kill_string) {
		/* insert the kill string at the current insertion point */
		result = tinyrl_insert_text(this, this->kill_string);
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_crlf(tinyrl_t * this, int key)
{
	tinyrl_crlf(this);
	this->done = true;
	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_up(tinyrl_t * this, int key)
{
	bool result = false;
	tinyrl_history_entry_t *entry = NULL;
	if (this->line == this->buffer) {
		/* go to the last history entry */
		entry = tinyrl_history_getlast(this->history, &this->hist_iter);
	} else {
		/* already traversing the history list so get previous */
		entry = tinyrl_history_getprevious(&this->hist_iter);
	}
	if (NULL != entry) {
		/* display the entry moving the insertion point
		 * to the end of the line 
		 */
		this->line = tinyrl_history_entry__get_line(entry);
		this->point = this->end = strlen(this->line);
		result = true;
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_down(tinyrl_t * this, int key)
{
	bool result = false;
	if (this->line != this->buffer) {
		/* we are not already at the bottom */
		/* the iterator will have been set up by the key_up() function */
		tinyrl_history_entry_t *entry =
			tinyrl_history_getnext(&this->hist_iter);
		if (NULL == entry) {
			/* nothing more in the history list */
			this->line = this->buffer;
		} else {
			this->line = tinyrl_history_entry__get_line(entry);
		}
		/* display the entry moving the insertion point
		 * to the end of the line 
		 */
		this->point = this->end = strlen(this->line);
		result = true;
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_left(tinyrl_t * this, int key)
{
	bool result = false;
	if (this->point > 0) {
		this->point--;
		result = true;
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_right(tinyrl_t * this, int key)
{
	bool result = false;
	if (this->point < this->end) {
		this->point++;
		result = true;
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_backspace(tinyrl_t * this, int key)
{
	bool result = false;
	if (this->point) {
		this->point--;
		tinyrl_delete_text(this, this->point, this->point + 1);
		result = true;
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_delete(tinyrl_t * this, int key)
{
	bool result = false;
	if (this->point < this->end) {
		tinyrl_delete_text(this, this->point, this->point + 1);
		result = true;
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_clear_screen(tinyrl_t * this, int key)
{
	tinyrl_vt100_clear_screen(this);
	tinyrl_vt100_cursor_home(this);
	tinyrl_reset_line_state(this);
	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_erase_line(tinyrl_t * this, int key)
{

	tinyrl_delete_text(this, 0, this->point);
	this->point = 0;
	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_escape(tinyrl_t * this, int key)
{
	switch (tinyrl_vt100_escape_decode(this)) {
	case tinyrl_vt100_CURSOR_UP:
		return tinyrl_key_up(this, key);
	case tinyrl_vt100_CURSOR_DOWN:
		return tinyrl_key_down(this, key);
	case tinyrl_vt100_CURSOR_LEFT:
		return tinyrl_key_left(this, key);
	case tinyrl_vt100_CURSOR_RIGHT:
		return tinyrl_key_right(this, key);
	case tinyrl_vt100_UNKNOWN:
		break;
	}
	return false;
}

/*-------------------------------------------------------- */
static void tinyrl_fini(tinyrl_t * this)
{
	/* delete the history session */
	tinyrl_history_delete(this->history);

	/* free up any dynamic strings */
	free(this->buffer);
	this->buffer = NULL;
	free(this->kill_string);
	this->kill_string = NULL;
	free(this->last_buffer);
	this->last_buffer = NULL;
}

/*-------------------------------------------------------- */
static void
tinyrl_init(tinyrl_t * this, FILE * instream, FILE * outstream,
	    unsigned stifle)
{
	int i;

	for (i = 0; i < NUM_HANDLERS; i++) {
		this->handlers[i] = tinyrl_key_default;
	}
	/* default handlers */
	this->handlers['\r'] = tinyrl_key_crlf;
	this->handlers['\n'] = tinyrl_key_crlf;
	this->handlers[CTRL('C')] = tinyrl_key_interrupt;
	this->handlers[BACKSPACE] = tinyrl_key_backspace;
	this->handlers[CTRL('H')] = tinyrl_key_backspace;
	this->handlers[CTRL('D')] = tinyrl_key_delete;
	this->handlers[ESCAPE] = tinyrl_key_escape;
	this->handlers[CTRL('L')] = tinyrl_key_clear_screen;
	this->handlers[CTRL('U')] = tinyrl_key_erase_line;
	this->handlers[CTRL('A')] = tinyrl_key_start_of_line;
	this->handlers[CTRL('E')] = tinyrl_key_end_of_line;
	this->handlers[CTRL('K')] = tinyrl_key_kill;
	this->handlers[CTRL('Y')] = tinyrl_key_yank;

	this->line = NULL;
	this->max_line_length = 0;
	this->prompt = NULL;
	this->buffer = NULL;
	this->buffer_size = 0;
	this->done = false;
	this->point = 0;
	this->end = 0;
	this->kill_string = NULL;
	this->echo_char = '\0';
	this->echo_enabled = true;
	this->isatty = isatty(fileno(instream));
	this->last_buffer = NULL;
	this->last_point = 0;

	this->istream = instream;
	this->ostream = outstream;

	/* create the history */
	this->history = tinyrl_history_new(stifle);
}

/*-------------------------------------------------------- */
int tinyrl_printf(const tinyrl_t * this, const char *fmt, ...)
{
	va_list args;
	int len;

	va_start(args, fmt);
	len = vfprintf(this->ostream, fmt, args);
	va_end(args);

	return len;
}

/*-------------------------------------------------------- */
void tinyrl_delete(tinyrl_t * this)
{
	assert(this);
	if (this) {
		/* let the object tidy itself up */
		tinyrl_fini(this);

		/* release the memory associate with this instance */
		free(this);
	}
}

/*-------------------------------------------------------- */

/*#####################################
 * EXPORTED INTERFACE
 *##################################### */
/*----------------------------------------------------------------------- */
static int tinyrl_getchar(const tinyrl_t * this)
{
	return getc(this->istream);
}

/*----------------------------------------------------------------------- */
static void tinyrl_internal_print(const tinyrl_t * this, const char *text)
{
	if (this->echo_enabled) {
		/* simply echo the line */
		tinyrl_printf(this, "%s", text);
	} else {
		/* replace the line with echo char if defined */
		if (this->echo_char) {
			unsigned i = strlen(text);
			while (i--) {
				tinyrl_printf(this, "%c", this->echo_char);
			}
		}
	}
}

/*----------------------------------------------------------------------- */
void tinyrl_redisplay(tinyrl_t * this)
{
	int delta;
	unsigned line_len, last_line_len, count;
	line_len = strlen(this->line);
	last_line_len = (this->last_buffer ? strlen(this->last_buffer) : 0);

	do {
		if (this->last_buffer) {
			delta = (int)(line_len - last_line_len);
			if (delta > 0) {
				count = (unsigned)delta;
				/* is the current line simply an extension of the previous one? */
				if (0 ==
				    strncmp(this->line, this->last_buffer,
					    last_line_len)) {
					/* output the line accounting for the echo behaviour */
					tinyrl_internal_print(this,
							      &this->
							      line[line_len -
							      count]);
					break;
				}
			} else if (delta < 0) {
				/* is the current line simply a deletion of some characters from the end? */
				if (0 ==
				    strncmp(this->line, this->last_buffer,
					    line_len)) {
					if (this->echo_enabled
					    || this->echo_char) {
						int shift =
							(int)(this->last_point -
							      this->point);
						/* just get the terminal to delete the characters */
						if (shift > 0) {
							count = (unsigned)shift;
							/* we've moved the cursor backwards */
							tinyrl_vt100_cursor_back
								(this, count);
						} else if (shift < 0) {
							count =
								(unsigned)-shift;
							/* we've moved the cursor forwards */
							tinyrl_vt100_cursor_forward
								(this, count);
						}
						/* now delete the characters */
						count = (unsigned)-delta;
						tinyrl_vt100_erase(this, count);
					}
					break;
				}
			} else {
				/* are the lines are the same content? */
				if (0 == strcmp(this->line, this->last_buffer)) {
					if (this->echo_enabled
					    || this->echo_char) {
						delta =
							(int)(this->point -
							      this->last_point);
						if (delta > 0) {
							count = (unsigned)delta;
							/* move the point forwards */
							tinyrl_vt100_cursor_forward
								(this, count);
						} else if (delta < 0) {
							count =
								(unsigned)-delta;
							/* move the cursor backwards */
							tinyrl_vt100_cursor_back
								(this, count);
						}
					}
					/* done for now */
					break;
				}
			}
		} else {
			/* simply display the prompt and the line */
			tinyrl_printf(this, "%s", this->prompt);
			tinyrl_internal_print(this, this->line);
			if (this->point < line_len) {
				/* move the cursor to the insertion point */
				tinyrl_vt100_cursor_back(this,
							 line_len -
							 this->point);
			}
			break;
		}
		/* 
		 * to have got this far we must have edited the middle of a line.
		 */
		if (this->last_point) {
			/* move to just after the prompt of the line */
			tinyrl_vt100_cursor_back(this, this->last_point);
		}
		/* erase the previous line */
		tinyrl_vt100_erase(this, strlen(this->last_buffer));

		/* output the line accounting for the echo behaviour */
		tinyrl_internal_print(this, this->line);

		delta = (int)(line_len - this->point);
		if (delta) {
			/* move the cursor back to the insertion point */
			tinyrl_vt100_cursor_back(this,
						 (strlen(this->line) -
						  this->point));
		}
	} /*lint -e717 */ while (0)	/*lint +e717 */
	;

	/* update the display */
	fflush(this->ostream);

	/* set up the last line buffer */
	free(this->last_buffer);
	this->last_buffer = strdup(this->line);
	this->last_point = this->point;
}

/*----------------------------------------------------------------------- */
tinyrl_t *tinyrl_new(FILE * instream, FILE * outstream, unsigned stifle)
{
	tinyrl_t *this = NULL;

	this = malloc(sizeof(tinyrl_t));
	if (NULL != this) {
		tinyrl_init(this, instream, outstream, stifle);
	}

	return this;
}

/*----------------------------------------------------------------------- */
char *tinyrl_readline(tinyrl_t * this, const char *prompt, void *context)
{
	/* initialise for reading a line */
	this->done = false;
	this->point = 0;
	this->end = 0;
	this->buffer = strdup("");
	this->buffer_size = strlen(this->buffer);
	this->line = this->buffer;
	this->prompt = prompt;
	this->context = context;

	if (this->isatty) {
		/* set the terminal into raw input mode */
		tty_set_raw_mode(this);

		tinyrl_reset_line_state(this);

		while (!this->done) {
			int key;
			/* update the display */
			tinyrl_redisplay(this);

			/* get a key */
			key = tinyrl_getchar(this);

			/* has the input stream terminated? */
			if (EOF != key) {
				/* call the handler for this key */
				if (!this->handlers[key](this, key)) {
					/* an issue has occured */
					tinyrl_ding(this);
				}

				if (this->done) {
					/*
					 * If the last character in the line (other than 
					 * the null) is a space remove it.
					 */
					if (this->end
					    && isspace(this->
						       line[this->end - 1])) {
						tinyrl_delete_text(this,
								   this->end -
								   1,
								   this->end);
					}
				}
			} else {
				/* time to finish the session */
				this->done = true;
				this->line = NULL;
			}
		}
		/* restores the terminal mode */
		tty_restore_mode(this);
	} else {
		/* This is a non-interactive set of commands */
		char *s = 0, buffer[80];
		size_t len = sizeof(buffer);

		/* manually reset the line state without redisplaying */
		free(this->last_buffer);
		this->last_buffer = NULL;

		while ((sizeof(buffer) == len) &&
		       (s = fgets(buffer, sizeof(buffer), this->istream))) {
			char *p;
			/* strip any spurious '\r' or '\n' */
			p = strchr(buffer, '\r');
			if (NULL == p) {
				p = strchr(buffer, '\n');
			}
			if (NULL != p) {
				*p = '\0';
			}
			/* skip any whitespace at the beginning of the line */
			if (0 == this->point) {
				while (*s && isspace(*s)) {
					s++;
				}
			}
			if (*s) {
				/* append this string to the input buffer */
				(void)tinyrl_insert_text(this, s);
				/* echo the command to the output stream */
				tinyrl_redisplay(this);
			}
			len = strlen(buffer) + 1;	/* account for the '\0' */
		}

		/*
		 * check against fgets returning null as either error or end of file.
		 * This is a measure to stop potential task spin on encountering an
		 * error from fgets.
		 */
		if (s == NULL || (this->line[0] == '\0' && feof(this->istream))) {
			/* time to finish the session */
			this->line = NULL;
		} else {
			tinyrl_crlf(this);
			this->done = true;
		}
	}
	/*
	 * duplicate the string for return to the client 
	 * we have to duplicate as we may be referencing a
	 * history entry or our internal buffer
	 */
	{
		char *result = this->line ? strdup(this->line) : NULL;

		/* free our internal buffer */
		free(this->buffer);
		this->buffer = NULL;

		if ((NULL == result) || '\0' == *result) {
			/* make sure we're not left on a prompt line */
			tinyrl_crlf(this);
		}
		return result;
	}
}

/*----------------------------------------------------------------------- */
/*
 * Ensure that buffer has enough space to hold len characters,
 * possibly reallocating it if necessary. The function returns true
 * if the line is successfully extended, false if not.
 */
static bool tinyrl_extend_line_buffer(tinyrl_t * this, unsigned len)
{
	bool result = true;
	if (this->buffer_size < len) {
		char *new_buffer;
		size_t new_len = len;

		/* 
		 * What we do depends on whether we are limited by
		 * memory or a user imposed limit.
		 */

		if (this->max_line_length == 0) {
			if (new_len < this->buffer_size + 10) {
				/* make sure we don't realloc too often */
				new_len = this->buffer_size + 10;
			}
			/* leave space for terminator */
			new_buffer = realloc(this->buffer, new_len + 1);

			if (NULL == new_buffer) {
				tinyrl_ding(this);
				result = false;
			} else {
				this->buffer_size = new_len;
				this->line = this->buffer = new_buffer;
				result = true;
			}
		} else {
			if (new_len < this->max_line_length) {

				/* Just reallocate once to the max size */
				new_buffer =
					realloc(this->buffer,
						this->max_line_length);

				if (NULL == new_buffer) {
					tinyrl_ding(this);
					result = false;
				} else {
					this->buffer_size =
						this->max_line_length - 1;
					this->line = this->buffer = new_buffer;
					result = true;
				}
			} else {
				tinyrl_ding(this);
				result = false;
			}
		}
	}
	return result;
}

/*----------------------------------------------------------------------- */
/*
 * Insert text into the line at the current cursor position.
 */
bool tinyrl_insert_text_len(tinyrl_t * this, const char *text, unsigned delta)
{
	/* 
	 * If the client wants to change the line ensure that the line and buffer
	 * references are in sync
	 */
	changed_line(this);

	if ((delta + this->end) > (this->buffer_size)) {
		/* extend the current buffer */
		if (!tinyrl_extend_line_buffer(this, this->end + delta)) {
			return false;
		}
	}

	if (this->point < this->end) {
		/* move the current text to the right (including the terminator) */
		memmove(&this->buffer[this->point + delta],
			&this->buffer[this->point],
			(this->end - this->point) + 1);
	} else {
		/* terminate the string */
		this->buffer[this->end + delta] = '\0';
	}

	/* insert the new text */
	strncpy(&this->buffer[this->point], text, delta);

	/* now update the indexes */
	this->point += delta;
	this->end += delta;

	return true;
}

bool tinyrl_insert_text(tinyrl_t * this, const char *text)
{
	return tinyrl_insert_text_len(this, text, strlen(text));
}

/*----------------------------------------------------------------------- */
/*
 * Delete the text in the interval [start, end-1] in the current line.
 * This adjusts the rl_point and rl_end indexes appropriately.
 */
void tinyrl_delete_text(tinyrl_t * this, unsigned start, unsigned end)
{
	unsigned delta;

	if (end == start)
		return;

	changed_line(this);

	/* move any text which is left, including terminator */
	delta = end - start;
	memmove(&this->buffer[start],
		&this->buffer[start + delta], this->end + 1 - end);
	this->end -= delta;

	/* now adjust the indexs */
	if (this->point > end) {
		/* move the insertion point back appropriately */
		this->point -= delta;
	} else if (this->point > start) {
		/* move the insertion point to the start */
		this->point = start;
	}
}

/*----------------------------------------------------------------------- */
bool tinyrl_bind_key(tinyrl_t * this, int key, tinyrl_key_func_t * fn)
{
	bool result = false;

	if ((key >= 0) && (key < NUM_HANDLERS)) {
		/* set the key handling function */
		this->handlers[key] = fn;
		result = true;
	}

	return result;
}

/*-------------------------------------------------------- */
void tinyrl_crlf(const tinyrl_t * this)
{
	tinyrl_printf(this, "\n");
}

/*-------------------------------------------------------- */
/*
 * Ring the terminal bell, obeying the setting of bell-style.
 */
void tinyrl_ding(const tinyrl_t * this)
{
	tinyrl_printf(this, "\x7");
	fflush(this->ostream);
}

/*-------------------------------------------------------- */
void tinyrl_reset_line_state(tinyrl_t * this)
{
	/* start from scratch */
	free(this->last_buffer);
	this->last_buffer = NULL;

	tinyrl_redisplay(this);
}

/*-------------------------------------------------------- */
void tinyrl_replace_line(tinyrl_t * this, const char *text, int clear_undo)
{
	size_t new_len = strlen(text);

	/* ignored for now */
	clear_undo = clear_undo;

	/* ensure there is sufficient space */
	if (tinyrl_extend_line_buffer(this, new_len)) {

		/* overwrite the current contents of the buffer */
		strcpy(this->buffer, text);

		/* set the insert point and end point */
		this->point = this->end = new_len;
	}
	tinyrl_redisplay(this);
}

/*-------------------------------------------------------- */
void *tinyrl__get_context(const tinyrl_t * this)
{
	return this->context;
}

/*--------------------------------------------------------- */
const char *tinyrl__get_line(const tinyrl_t * this)
{
	return this->line;
}

/*--------------------------------------------------------- */
unsigned tinyrl__get_point(const tinyrl_t * this)
{
	return this->point;
}

/*--------------------------------------------------------- */
unsigned tinyrl__get_end(const tinyrl_t * this)
{
	return this->end;
}

/*--------------------------------------------------------- */
unsigned tinyrl__get_width(const tinyrl_t * this)
{
	/* hard code until we suss out how to do it properly */
	return 80;
}

/*--------------------------------------------------------- */
tinyrl_history_t *tinyrl__get_history(const tinyrl_t * this)
{
	return this->history;
}

/*--------------------------------------------------------- */
void tinyrl_done(tinyrl_t * this)
{
	this->done = true;
}

/*--------------------------------------------------------- */
void tinyrl_enable_echo(tinyrl_t * this)
{
	this->echo_enabled = true;
}

/*--------------------------------------------------------- */
void tinyrl_disable_echo(tinyrl_t * this, char echo_char)
{
	this->echo_enabled = false;
	this->echo_char = echo_char;
}

/*--------------------------------------------------------- */
void tinyrl__set_istream(tinyrl_t * this, FILE * istream)
{
	this->istream = istream;
	this->isatty = isatty(fileno(istream));
}

/*-------------------------------------------------------- */
bool tinyrl__get_isatty(const tinyrl_t * this)
{
	return this->isatty;
}

/*-------------------------------------------------------- */
FILE *tinyrl__get_istream(const tinyrl_t * this)
{
	return this->istream;
}

/*-------------------------------------------------------- */
FILE *tinyrl__get_ostream(const tinyrl_t * this)
{
	return this->ostream;
}

/*-------------------------------------------------------- */
const char *tinyrl__get_prompt(const tinyrl_t * this)
{
	return this->prompt;
}

/*--------------------------------------------------------- */
void tinyrl_limit_line_length(tinyrl_t * this, unsigned length)
{
	this->max_line_length = length;
}

/*--------------------------------------------------------- */
