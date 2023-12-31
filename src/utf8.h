#ifndef ANDY_UTF8_H
#define ANDY_UTF8_H

#include <stddef.h>

#include "uni.h"

rune_t utf8iter(const char *, size_t *);
char *utf8trim(char *);
bool utf8all(const char *, bool (*)(rune_t));

#endif /* !ANDY_UTF8_H */
