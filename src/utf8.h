#ifndef ANDY_UTF8_H
#define ANDY_UTF8_H

#include <uchar.h>

#include "uni.h"

/* Return the rune at index n in the given string, and set n to the index of the
   next rune.  If an invalid byte-sequence is detected, UNI_REPL_CHAR is
   returned. */
rune utf8iter(const char8_t *, size_t *n);

/* Remove leading- and trailing-whitespace from the given string, and return a
   pointer to the beginning of the string.  This function destructively modifies
   the input string, and the return value does not need to equal the input
   pointer. */
char8_t *utf8trim(char8_t *);

/* Return whether the given predicate function returns true for all runes in the
   given string. */
bool utf8all(const char8_t *, bool (*)(rune));

/* Return a pointer to first rune in the given string for which the given
   predicate function is false. */
char8_t *utf8fskip(const char8_t *, bool (*)(rune));

/* Return the number of bytes occupied by the given rune when UTF-8 encoded. */
int utf8wdth(rune);

#endif /* !ANDY_UTF8_H */
