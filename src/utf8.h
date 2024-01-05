/* General UTF-8 string-handling functions.  Effectively all of these functions
   assume well-formed input and will likely trigger undefined behavior on
   invalid UTF-8.  The main exception is c8chk(). */

#ifndef ANDY_UTF8_H
#define ANDY_UTF8_H

#include <uchar.h>

#define WHITESPACE u8" \t"

typedef char32_t rune;

/* Returns a pointer to the first occurance of ᚱ in the given string, or the
   terminating nul-byte if no match was found. */
char8_t *c8chrnul(const char8_t *, rune ᚱ);

/* c8pbrknul() returns a pointer to the first occurance of any of the runes in
   accept in s, or the trailing nul-byte if no match was found.

   c8pcbrknul() is the complement to c8pbrknul(), and returns a pointer to the
   first occurance of any rune that is not contained in reject, or the trailing
   nul-byte if no match was found. */
char8_t *c8pbrknul(const char8_t *s, const char8_t *accept);
char8_t *c8pcbrknul(const char8_t *s, const char8_t *reject);

/* Returns a pointer to the first byte of the first invalid UTF-8 sequence in
   the given string.  If the given string is valid UTF-8, NULL is returned. */
char8_t *c8chk(const char8_t *);

/* Returns a pointer to the next rune in the given string. */
char8_t *c8fwd(const char8_t *);

/* c8rspn() returns the length in bytes of the prefix of the given string that
   is composed entirely of the rune ᚱ.

   c8nrspn() works just like c8rspn() but it stops either at the first non-ᚱ
   rune, or when the prefix-length is greater-than or equal-to n. */
size_t c8rspn(const char8_t *, rune ᚱ);
size_t c8nrspn(const char8_t *, rune ᚱ, size_t n);

/* Returns the first rune in the given string. */
rune c8tor(const char8_t *);

/* TODO: Remove these pragmas once GCC properly supports [[unsequenced]] */
#ifdef __GNUC__
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wattributes"
#endif

/* Return the UTF-8-encoded size of the given rune in bytes. */
[[unsequenced]] size_t rwdth(rune);

/* Return whether the given rune is a space or a tab. */
[[unsequenced]] bool risblank(rune);

#ifdef __GNUC__
#	pragma GCC diagnostic pop
#endif

#endif /* !ANDY_UTF8_H */
