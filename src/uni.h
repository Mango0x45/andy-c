#ifndef ANDY_UNI_H
#define ANDY_UNI_H

#include <stdint.h>

/* U+FFFD REPLACEMENT CHARACTER; intended for use when invalid UTF-8 sequences
   are detected. */
#define UNI_REPL_CHAR ((rune_t)0xFFFD)

typedef uint32_t rune_t;

/* Assert whether the given rune is a unicode whitespace character. */
bool unispace(rune_t);

#endif /* !ANDY_UNI_H */
