#ifndef ANDY_UTF8_H
#define ANDY_UTF8_H

#include <stddef.h>
#include <stdint.h>

#define UNI_REPL_CHAR 0xFFFD

typedef uint32_t rune_t;

rune_t utf8iter(const char *, size_t *);

#endif /* !ANDY_UTF8_H */
