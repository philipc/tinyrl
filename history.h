/**
  \ingroup tinyrl
  \defgroup tinyrl_history history
  @{

  \brief This class handles the maintenance of a historical list of command lines.

*/
#ifndef _tinyrl_history_h
#define _tinyrl_history_h

#include <stdbool.h>

/**************************************
 * tinyrl_history class interface
 ************************************** */

struct tinyrl;

struct tinyrl_history *tinyrl_history_new(struct tinyrl *tinyrl, unsigned limit);

void tinyrl_history_delete(struct tinyrl_history *history);
void tinyrl_history_add(struct tinyrl_history *history, const char *line);

/*
   HISTORY LIST MANAGEMENT 
   */
void tinyrl_history_remove(struct tinyrl_history *history, unsigned offset);
void tinyrl_history_clear(struct tinyrl_history *history);
void tinyrl_history_limit(struct tinyrl_history *history, unsigned limit);

const char *tinyrl_history_get(const struct tinyrl_history *history,
				      unsigned offset);
size_t tinyrl_history_length(const struct tinyrl_history *history);

#endif				/* _tinyrl_history_h */
/** @} tinyrl_history */
