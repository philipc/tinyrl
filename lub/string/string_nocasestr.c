/*
 * string_nocasestr.c
 *
 * Find a string within another string in a case insensitive manner
 */
 #include "private.h"
 
 #include <ctype.h>
 
/*--------------------------------------------------------- */
const char *
lub_string_nocasestr(const char *cs,
                     const char *ct)
{
    const char *p      = NULL;
    const char *result = NULL;
    
    while(*cs)
    {
        const char *q = cs;

        p = ct;
        while(*p && *q && (tolower(*p) == tolower(*q)))
        {
            p++,q++;
        }
        if(0 == *p)
        { 
            break;
        }
        cs++;
    }
    if(p && !*p)
    {
        /* we've found the first match of ct within cs */
        result = cs;
    }
    return result;
}
/*--------------------------------------------------------- */
