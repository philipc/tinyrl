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

#define KEYMAP_SIZE 256

struct tinyrl_keymap {
	tinyrl_key_func_t *handler[KEYMAP_SIZE];
	struct tinyrl_keymap *keymap[KEYMAP_SIZE];
	void *context[KEYMAP_SIZE];
};

/* define the class member data and virtual methods */
struct tinyrl {
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
	struct tinyrl_keymap *keymap;

	char echo_char;
	bool echo_enabled;
	struct termios default_termios;
	bool isatty;
	char *last_buffer;	/* hold record of the previous 
				   buffer for redisplay purposes */
	unsigned last_point;	/* hold record of the previous 
				   cursor position for redisplay purposes */
};

#define ESCAPESEQ "\x1b["
#define ESCAPE 27
#define BACKSPACE 127

/*--------------------------------------------------------- */
static void _tinyrl_vt100_setInputNonBlocking(const struct tinyrl *this)
{
#if defined(STDIN_FILENO)
	int flags = (fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
	fcntl(STDIN_FILENO, F_SETFL, flags);
#endif				/* STDIN_FILENO */
}

/*--------------------------------------------------------- */
static void _tinyrl_vt100_setInputBlocking(const struct tinyrl *this)
{
#if defined(STDIN_FILENO)
	int flags = (fcntl(STDIN_FILENO, F_GETFL, 0) & ~O_NONBLOCK);
	fcntl(STDIN_FILENO, F_SETFL, flags);
#endif				/* STDIN_FILENO */
}

/*-------------------------------------------------------- */
static void tinyrl_vt100_clear_screen(const struct tinyrl *this)
{
	tinyrl_printf(this, "\x1b[2J");
}

/*-------------------------------------------------------- */
static void tinyrl_vt100_cursor_forward(const struct tinyrl *this, unsigned count)
{
	tinyrl_printf(this, "\x1b[%dC", count);
}

/*-------------------------------------------------------- */
static void tinyrl_vt100_cursor_back(const struct tinyrl *this, unsigned count)
{
	tinyrl_printf(this, "\x1b[%dD", count);
}

/*-------------------------------------------------------- */
static void tinyrl_vt100_cursor_home(const struct tinyrl *this)
{
	tinyrl_printf(this, "\x1b[H");
}

/*-------------------------------------------------------- */
static void tinyrl_vt100_erase(const struct tinyrl *this, unsigned count)
{
	tinyrl_printf(this, "\x1b[%dP", count);
}

/*----------------------------------------------------------------------- */
static void tty_set_raw_mode(struct tinyrl *this)
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
static void tty_restore_mode(const struct tinyrl *this)
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
static void changed_line(struct tinyrl *this)
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
static bool tinyrl_key_default(void *context, int key)
{
	struct tinyrl *this = context;
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
static bool tinyrl_key_interrupt(void *context, int key)
{
	struct tinyrl *this = context;

	tinyrl_delete_text(this, 0, this->end);
	this->done = true;

	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_start_of_line(void *context, int key)
{
	struct tinyrl *this = context;

	/* set the insertion point to the start of the line */
	this->point = 0;
	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_end_of_line(void *context, int key)
{
	struct tinyrl *this = context;

	/* set the insertion point to the end of the line */
	this->point = this->end;
	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_kill(void *context, int key)
{
	struct tinyrl *this = context;

	/* release any old kill string */
	free(this->kill_string);

	/* store the killed string */
	this->kill_string = strdup(&this->buffer[this->point]);

	/* delete the text to the end of the line */
	tinyrl_delete_text(this, this->point, this->end);
	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_yank(void *context, int key)
{
	struct tinyrl *this = context;
	bool result = false;
	if (this->kill_string) {
		/* insert the kill string at the current insertion point */
		result = tinyrl_insert_text(this, this->kill_string);
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_crlf(void *context, int key)
{
	struct tinyrl *this = context;

	tinyrl_crlf(this);
	this->done = true;
	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_left(void *context, int key)
{
	struct tinyrl *this = context;
	bool result = false;
	if (this->point > 0) {
		this->point--;
		result = true;
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_right(void *context, int key)
{
	struct tinyrl *this = context;
	bool result = false;
	if (this->point < this->end) {
		this->point++;
		result = true;
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_backspace(void *context, int key)
{
	struct tinyrl *this = context;
	bool result = false;
	if (this->point) {
		this->point--;
		tinyrl_delete_text(this, this->point, this->point + 1);
		result = true;
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_delete(void *context, int key)
{
	struct tinyrl *this = context;
	bool result = false;
	if (this->point < this->end) {
		tinyrl_delete_text(this, this->point, this->point + 1);
		result = true;
	}
	return result;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_clear_screen(void *context, int key)
{
	struct tinyrl *this = context;

	tinyrl_vt100_clear_screen(this);
	tinyrl_vt100_cursor_home(this);
	tinyrl_reset_line_state(this);
	return true;
}

/*-------------------------------------------------------- */
static bool tinyrl_key_erase_line(void *context, int key)
{
	struct tinyrl *this = context;

	tinyrl_delete_text(this, 0, this->point);
	this->point = 0;
	return true;
}

/*-------------------------------------------------------- */
static struct tinyrl_keymap *tinyrl_keymap_new()
{
	struct tinyrl_keymap *keymap;
	int i;

	keymap = malloc(sizeof(*keymap));

	for (i = 0; i < KEYMAP_SIZE; i++) {
		keymap->handler[i] = NULL;
		keymap->keymap[i] = NULL;
		keymap->context[i] = NULL;
	}

	return keymap;
}

static void tinyrl_keymap_free(struct tinyrl_keymap *keymap)
{
	int i;

	for (i = 0; i < KEYMAP_SIZE; i++)
		if (keymap->keymap[i])
			tinyrl_keymap_free(keymap->keymap[i]);
	free(keymap);
}

/*-------------------------------------------------------- */
static void tinyrl_fini(struct tinyrl *this)
{
	/* free up any dynamic strings */
	free(this->buffer);
	this->buffer = NULL;
	free(this->kill_string);
	this->kill_string = NULL;
	free(this->last_buffer);
	this->last_buffer = NULL;
	tinyrl_keymap_free(this->keymap);
}

/*-------------------------------------------------------- */
static void
tinyrl_init(struct tinyrl *this, FILE * instream, FILE * outstream)
{
	int i;

	this->keymap = tinyrl_keymap_new();
	for (i = 32; i < 256; i++)
		tinyrl_bind_key(this, i, tinyrl_key_default, this);
	tinyrl_bind_key(this, '\r', tinyrl_key_crlf, this);
	tinyrl_bind_key(this, '\n', tinyrl_key_crlf, this);
	tinyrl_bind_key(this, CTRL('C'), tinyrl_key_interrupt, this);
	tinyrl_bind_key(this, BACKSPACE, tinyrl_key_backspace, this);
	tinyrl_bind_key(this, CTRL('H'), tinyrl_key_backspace, this);
	tinyrl_bind_key(this, CTRL('D'), tinyrl_key_delete, this);
	tinyrl_bind_key(this, CTRL('L'), tinyrl_key_clear_screen, this);
	tinyrl_bind_key(this, CTRL('U'), tinyrl_key_erase_line, this);
	tinyrl_bind_key(this, CTRL('A'), tinyrl_key_start_of_line, this);
	tinyrl_bind_key(this, CTRL('E'), tinyrl_key_end_of_line, this);
	tinyrl_bind_key(this, CTRL('K'), tinyrl_key_kill, this);
	tinyrl_bind_key(this, CTRL('Y'), tinyrl_key_yank, this);
	tinyrl_bind_special(this, TINYRL_KEY_RIGHT, tinyrl_key_right, this);
	tinyrl_bind_special(this, TINYRL_KEY_LEFT, tinyrl_key_left, this);

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
}

/*-------------------------------------------------------- */
int tinyrl_printf(const struct tinyrl *this, const char *fmt, ...)
{
	va_list args;
	int len;

	va_start(args, fmt);
	len = vfprintf(this->ostream, fmt, args);
	va_end(args);

	return len;
}

/*-------------------------------------------------------- */
void tinyrl_delete(struct tinyrl *this)
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
static int tinyrl_getchar(const struct tinyrl *this)
{
	return getc(this->istream);
}

/*----------------------------------------------------------------------- */
static void tinyrl_internal_print(const struct tinyrl *this, const char *text)
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
void tinyrl_redisplay(struct tinyrl *this)
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
struct tinyrl *tinyrl_new(FILE * instream, FILE * outstream)
{
	struct tinyrl *this = NULL;

	this = malloc(sizeof(*this));
	if (NULL != this) {
		tinyrl_init(this, instream, outstream);
	}

	return this;
}

/*----------------------------------------------------------------------- */
/* Call the handler for the longest matching key sequence.
 * Note: if there is a partial match, then the extra keys are discarded.  This
 * shouldn't matter in practice.
 */
static void tinyrl_handle_key(struct tinyrl *this, int key)
{
	struct tinyrl_keymap *keymap;
	tinyrl_key_func_t *handler;
	void *context;
	int c;

	handler = NULL;
	context = NULL;
	keymap = this->keymap;
	c = key;
	for (;;) {
		if (keymap->handler[c]) {
			handler = keymap->handler[c];
			context = keymap->context[c];
		}
		keymap = keymap->keymap[c];
		if (!keymap)
			break;

		_tinyrl_vt100_setInputNonBlocking(this);
		c = getc(this->istream);
		_tinyrl_vt100_setInputBlocking(this);
		if (c == EOF)
			break;
	}

	if (!handler || !handler(context, key)) {
		/* an issue has occured */
		tinyrl_ding(this);
	}
}

char *tinyrl_readline(struct tinyrl *this, const char *prompt)
{
	/* initialise for reading a line */
	this->done = false;
	this->point = 0;
	this->end = 0;
	this->buffer = strdup("");
	this->buffer_size = strlen(this->buffer);
	this->line = this->buffer;
	this->prompt = prompt;

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
				tinyrl_handle_key(this, key);

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
static bool tinyrl_extend_line_buffer(struct tinyrl *this, unsigned len)
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
bool tinyrl_insert_text_len(struct tinyrl *this, const char *text, unsigned delta)
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

bool tinyrl_insert_text(struct tinyrl *this, const char *text)
{
	return tinyrl_insert_text_len(this, text, strlen(text));
}

/*----------------------------------------------------------------------- */
/*
 * Delete the text in the interval [start, end-1] in the current line.
 * This adjusts the rl_point and rl_end indexes appropriately.
 */
void tinyrl_delete_text(struct tinyrl *this, unsigned start, unsigned end)
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
static void tinyrl_bind_keyseq(struct tinyrl *this, const char *seq,
			       tinyrl_key_func_t *handler, void *context)
{
	struct tinyrl_keymap *keymap;
	unsigned char key;

	if (!*seq)
		return;

	keymap = this->keymap;
	key = *seq++;

	while (*seq) {
		if (!keymap->keymap[key])
			keymap->keymap[key] = tinyrl_keymap_new();
		keymap = keymap->keymap[key];
		key = *seq++;
	}

	keymap->handler[key] = handler;
	keymap->context[key] = context;
}

void tinyrl_bind_special(struct tinyrl *this, enum tinyrl_key key,
			 tinyrl_key_func_t *handler, void *context)
{
	switch (key) {
	case TINYRL_KEY_UP:
		tinyrl_bind_keyseq(this, ESCAPESEQ "A", handler, context);
		break;
	case TINYRL_KEY_DOWN:
		tinyrl_bind_keyseq(this, ESCAPESEQ "B", handler, context);
		break;
	case TINYRL_KEY_LEFT:
		tinyrl_bind_keyseq(this, ESCAPESEQ "D", handler, context);
		break;
	case TINYRL_KEY_RIGHT:
		tinyrl_bind_keyseq(this, ESCAPESEQ "C", handler, context);
		break;
	}
}

void tinyrl_bind_key(struct tinyrl *this, unsigned char key,
		     tinyrl_key_func_t *handler, void *context)
{
	this->keymap->handler[key] = handler;
	this->keymap->context[key] = context;
}

/*-------------------------------------------------------- */
void tinyrl_crlf(const struct tinyrl *this)
{
	tinyrl_printf(this, "\n");
}

/*-------------------------------------------------------- */
/*
 * Ring the terminal bell, obeying the setting of bell-style.
 */
void tinyrl_ding(const struct tinyrl *this)
{
	tinyrl_printf(this, "\x7");
	fflush(this->ostream);
}

/*-------------------------------------------------------- */
void tinyrl_reset_line_state(struct tinyrl *this)
{
	/* start from scratch */
	free(this->last_buffer);
	this->last_buffer = NULL;

	tinyrl_redisplay(this);
}

/*-------------------------------------------------------- */
void tinyrl_set_line(struct tinyrl *this, const char *text)
{
	this->line = text ?: this->buffer;
	this->point = this->end = strlen(this->line);
}

/*-------------------------------------------------------- */
void tinyrl_replace_line(struct tinyrl *this, const char *text)
{
	size_t new_len = strlen(text);

	if (tinyrl_extend_line_buffer(this, new_len)) {
		strcpy(this->buffer, text);
		this->point = this->end = new_len;
	}
	tinyrl_redisplay(this);
}

/*--------------------------------------------------------- */
const char *tinyrl_get_line(const struct tinyrl *this)
{
	return this->line;
}

/*--------------------------------------------------------- */
unsigned tinyrl_get_point(const struct tinyrl *this)
{
	return this->point;
}

/*--------------------------------------------------------- */
unsigned tinyrl__get_width(const struct tinyrl *this)
{
	/* hard code until we suss out how to do it properly */
	return 80;
}

/*--------------------------------------------------------- */
void tinyrl_done(struct tinyrl *this)
{
	this->done = true;
}

/*--------------------------------------------------------- */
void tinyrl_enable_echo(struct tinyrl *this)
{
	this->echo_enabled = true;
}

/*--------------------------------------------------------- */
void tinyrl_disable_echo(struct tinyrl *this, char echo_char)
{
	this->echo_enabled = false;
	this->echo_char = echo_char;
}

/*-------------------------------------------------------- */
bool tinyrl__get_isatty(const struct tinyrl *this)
{
	return this->isatty;
}

/*--------------------------------------------------------- */
void tinyrl_limit_line_length(struct tinyrl *this, unsigned length)
{
	this->max_line_length = length;
}

/*--------------------------------------------------------- */
