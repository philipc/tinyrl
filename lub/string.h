/*
 * string.h
 */
/**
\ingroup lub
\defgroup lub_string string
@{

\brief This utility provides some simple string manipulation functions which
augment those found in the standard ANSI-C library.

As a rule of thumb if a function returns "char *" then the calling client becomes responsible for invoking 
free() to release the dynamically allocated memory.

If a "const char *" is returned then the client has no responsiblity for releasing memory.
*/
/*---------------------------------------------------------------
 * HISTORY
 * 7-Dec-2004		Graeme McKerrell	
 *    Updated to use the "lub" prefix
 * 6-Feb-2004		Graeme McKerrell	
 *    removed init_fn type definition and parameter, the client had
 *    more flexiblity in defining their own initialisation operation with
 *    arguments rather than use a "one-size-fits-all" approach.
 *    Modified blockpool structure to support FIFO block allocation.
 * 23-Jan-2004		Graeme McKerrell	
 *    Initial version
 *---------------------------------------------------------------
 * Copyright (C) 2004 3Com Corporation. All Rights Reserved.
 *--------------------------------------------------------------- */
#ifndef _lub_string_h
#define _lub_string_h

#include <stddef.h>

/**
 * This operation returns a pointer to the last (space separated) word in the
 * specified string.
 *
 * \pre 
 * - none
 * 
 * \return 
 * A pointer to the last word in the string.
 *
 * \post 
 * - none
 */
const char *
    lub_string_suffix(
        /**
         * The string from which to extract a suffix 
         */
        const char *string
    );

/**
 * This operation compares string cs to string ct in a case insensitive manner.
 *
 * \pre 
 * - none
 * 
 * \return 
 * - < 0 if cs < ct
 * -   0 if cs == ct
 * - > 0 if cs > ct
 *
 * \post 
 * - none
 */
int
    lub_string_nocasecmp(
        /**
         * The first string for the comparison
         */
        const char *cs,
        /**
         * The second string for the comparison 
         */
        const char *ct
    );
/**
 * This operation performs a case insensitive search for a substring within
 * another string.
 *
 * \pre 
 * - none
 * 
 * \return
 * pointer to first occurance of a case insensitive version of the string ct, 
 * or NULL if not present.
 *
 * \post 
 * - none
 */
const char *
    lub_string_nocasestr(
        /**
         * The string within which to find a substring
         */
        const char *cs,
        /**
         * The substring for which to search
         */
        const char *ct
    );
        
#endif /* _lub_string_h */
/** @} */

