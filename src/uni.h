#ifndef ANDY_UNI_H
#define ANDY_UNI_H

#include <uchar.h>

typedef char32_t rune_t;

/* U+FFFD REPLACEMENT CHARACTER; intended for use when invalid UTF-8 sequences
   are detected. */
constexpr rune_t UNI_REPL_CHAR = U'�';

/* Assert whether the given rune is a unicode whitespace character. */
[[gnu::const]] bool unispace(rune_t);

#endif /* !ANDY_UNI_H */
