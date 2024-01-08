/* General UTF-8 string-handling functions.  Effectively all of these functions
   assume well-formed input and will likely trigger undefined behavior on
   invalid UTF-8.  The main exception is c8chk(). */

#ifndef ANDY_UTF8_H
#define ANDY_UTF8_H

#include <uchar.h>

#include "util.h"

#define WHITESPACE u8" \t"

typedef char32_t rune;

/* Returns a pointer to the first occurance of ᚱ in the given string, or the
   terminating nul-byte if no match was found. */
[[nodiscard, nonulls]] char8_t *c8chrnul(const char8_t *, rune ᚱ);

/* c8pbrknul() returns a pointer to the first occurance of any of the runes in
   accept in s, or the trailing nul-byte if no match was found.

   c8pcbrknul() is the complement to c8pbrknul(), and returns a pointer to the
   first occurance of any rune that is not contained in reject, or the trailing
   nul-byte if no match was found. */
[[nodiscard, nonulls]] char8_t *c8pbrknul(const char8_t *s,
                                          const char8_t *accept);
[[nodiscard, nonulls]] char8_t *c8pcbrknul(const char8_t *s,
                                           const char8_t *reject);

/* Returns a pointer to the first byte of the first invalid UTF-8 sequence in
   the given string.  If the given string is valid UTF-8, NULL is returned. */
[[nodiscard, nonnull]] char8_t *c8chk(const char8_t *);

/* c8fwd() returns a pointer to the next codepoint in the given string. c8gfwd()
   is similar but works on graphemes instead of runes. */
[[nodiscard, nonulls]] char8_t *c8fwd(const char8_t *);
[[nodiscard, nonulls]] char8_t *c8gfwd(const char8_t *);

/* c8rspn() returns the length in bytes of the prefix of the given string that
   is composed entirely of the rune ᚱ.

   c8nrspn() works just like c8rspn() but it stops either at the first non-ᚱ
   rune, or when the prefix-length is greater-than or equal-to n. */
[[nodiscard, nonnull]] size_t c8rspn(const char8_t *, rune ᚱ);
[[nodiscard, nonnull]] size_t c8nrspn(const char8_t *, rune ᚱ, size_t n);

/* Returns the first rune in the given string. */
[[nodiscard, nonnull]] rune c8tor(const char8_t *);

/* TODO: Remove these pragmas once GCC properly supports [[unsequenced]] */
#ifdef __GNUC__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wattributes"
#endif

/* Return the UTF-8-encoded size of the given rune in bytes. */
[[nodiscard, unsequenced]] size_t rwdth(rune);

/* risstart() and riscont() test to see if the given rune has the unicode
   XID_Start or XID_Continue properties respectively. */
[[nodiscard, unsequenced]] bool risstart(rune);
[[nodiscard, unsequenced]] bool riscont(rune);

#ifdef __GNUC__
#	pragma GCC diagnostic pop
#endif

#endif /* !ANDY_UTF8_H */
