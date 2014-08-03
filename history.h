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

extern struct tinyrl_history *tinyrl_history_new(unsigned limit);

extern void tinyrl_history_delete(struct tinyrl_history * instance);
extern void tinyrl_history_add(struct tinyrl_history * instance, const char *line);

/*
   HISTORY LIST MANAGEMENT 
   */
extern void tinyrl_history_remove(struct tinyrl_history *instance, unsigned offset);
extern void tinyrl_history_clear(struct tinyrl_history * instance);
extern void tinyrl_history_limit(struct tinyrl_history * instance, unsigned limit);

extern const char *tinyrl_history_get(const struct tinyrl_history *instance,
				      unsigned offset);
extern size_t tinyrl_history_length(const struct tinyrl_history *instance);

#endif				/* _tinyrl_history_h */
/** @} tinyrl_history */
