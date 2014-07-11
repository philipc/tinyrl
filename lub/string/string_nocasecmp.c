/*
 * string_nocasecmp.c
 */
 #include "private.h"

 #include <ctype.h>
 
/*--------------------------------------------------------- */
int
lub_string_nocasecmp(const char *cs,
                     const char *ct)
{
    int result = 0;
    while( (0==result) && *cs && *ct)
    {
        int s = tolower(*cs++);
        int t = tolower(*ct++);

        result = s - t;
    }
    if(0 == result)
    {
        /* account for different string lengths */
        result = *cs - *ct;
    }
    return result;
}
/*--------------------------------------------------------- */
