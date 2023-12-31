#ifndef ANDY_UNI_H
#define ANDY_UNI_H

#include <stdint.h>

#define UNI_REPL_CHAR 0xFFFD

typedef uint32_t rune_t;

bool unispace(rune_t);

#endif /* !ANDY_UNI_H */
