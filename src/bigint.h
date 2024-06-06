#ifndef ANDY_BIGINT_H
#define ANDY_BIGINT_H

#include <mbstring.h>

/* Test if the given string represents a big integer.  A big integer is any
   string that matches the following regular expression:

       (-?[1-9][0-9]*|0)

   -0 and non-zero numbers with leading zeros are disallowed because we want to
   retain a 1:1 mapping of keys and values since we store everything in
   hashmaps.
 */
bool isbigint(struct u8view);

/* A comparison function suitable to be passed to qsort() to sort a list of big
   integers */
int bigintcmp(const void *, const void *);

#endif /* !ANDY_BIGINT_H */
