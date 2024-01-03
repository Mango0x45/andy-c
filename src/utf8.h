#ifndef ANDY_UTF8_H
#define ANDY_UTF8_H

#include <uchar.h>

typedef char32_t rune;

/* U+FFFD REPLACEMENT CHARACTER; intended for use when invalid UTF-8 sequences
   are detected. */
constexpr rune REPL_CHAR = U'�';

/* Return the first rune in the given null-terminated string.  If an invalid
   byte-sequence is detected, REPL_CHAR is returned. */
rune utf8peek(const char8_t *);

/* Return the first rune in the null-terminated string pointed to by the given
   pointer, and increment the given string to point to the next rune in the
   string.  If an invalid byte-sequence is detected, REPL_CHAR is returned.
 */
rune utf8next(const char8_t **);

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

/* Just like strchr() and strchrnul(), but these functions accept a rune instead
   of an int.  If the needle fits in a char, these functions will call the more
   optimal strchr() or strchrnul() functions automatically.  A strchrnul()
   implementation is provided on systems that do not ship it. */
char8_t *utf8chr(const char8_t *, rune);
char8_t *utf8chrnul(const char8_t *, rune);

/* utf8pfx() returns the length of the prefix of the given string in bytes
   consisting of the given rune.  This is similar to strspn() but only accepts
   one rune instead of a string of runes.

   utf8npfx() is the same as utf8pfx() but it only checks the first n bytes. */
size_t utf8pfx(const char8_t *, rune);
size_t utf8npfx(const char8_t *, rune, size_t n);

/* TODO: Remove these pragmas once GCC properly supports [[unsequenced]] */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

/* Just like isblank() from ctype.h but for runes. */
[[unsequenced]] bool risblank(rune);

#pragma GCC diagnostic pop

#endif /* !ANDY_UTF8_H */
