#ifndef ANDY_UTF8_H
#define ANDY_UTF8_H

#include <stddef.h>

#include "uni.h"

/* Return the rune at index n in the given string, and set n to the index of the
   next rune.  If an invalid byte-sequence is detected, UNI_REPL_CHAR is
   returned. */
rune_t utf8iter(const char *, size_t *n);

/* Remove leading- and trailing-whitespace from the given string, and return a
   pointer to the beginning of the string.  This function destructively modifies
   the input string, and the return value does not need to equal the input
   pointer. */
char *utf8trim(char *);

/* Return whether the given predicate function returns true for all runes in the
   given string. */
bool utf8all(const char *, bool (*)(rune_t));

/* Return a pointer to first rune in the given string for which the given
   predicate function is false. */
char *utf8skipf(const char *, bool (*)(rune_t));

#endif /* !ANDY_UTF8_H */
