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
typedef struct _tinyrl_history tinyrl_history_t;

extern tinyrl_history_t *tinyrl_history_new(unsigned stifle);

extern void tinyrl_history_delete(tinyrl_history_t * instance);
extern void tinyrl_history_add(tinyrl_history_t * instance, const char *line);

/*
   HISTORY LIST MANAGEMENT 
   */
extern void tinyrl_history_remove(tinyrl_history_t *instance, unsigned offset);
extern void tinyrl_history_clear(tinyrl_history_t * instance);
extern void tinyrl_history_stifle(tinyrl_history_t * instance, unsigned stifle);
extern unsigned tinyrl_history_unstifle(tinyrl_history_t * instance);
extern bool tinyrl_history_is_stifled(const tinyrl_history_t * instance);

extern const char *tinyrl_history_get(const tinyrl_history_t *instance,
				      unsigned offset);
extern size_t tinyrl_history_length(const tinyrl_history_t *instance);

#endif				/* _tinyrl_history_h */
/** @} tinyrl_history */
