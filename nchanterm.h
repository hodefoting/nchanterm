
#ifndef NCHANTERM_H

/* nchanterm is a "single file C library" meaning that it can be dropped into
 * a project and directly #included from a .c file (if there is only one
 * compilation unit using it). By setting this #define we make the included
 * .c file strip out the code leaving only the function declarations.
 */
#define NCHANTERM_HEADER_ONLY
#include "nchanterm.c"
#endif
