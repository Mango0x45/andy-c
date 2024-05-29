#ifndef ANDY_ERROR_H
#define ANDY_ERROR_H

#include <mbstring.h>

void errinit(void);
void diagemit(const char *, const char *, const char8_t *, struct u8view,
              size_t, const char *, ...);

#define erremit(file, base, hl, off, fmt, ...)                                 \
	diagemit("error", (file), (base), (hl), (off),                             \
	         (fmt)__VA_OPT__(, ) __VA_ARGS__)
#define warnemit(file, base, hl, off, fmt, ...)                                \
	diagemit("warning", (file), (base), (hl), (off),                           \
	         (fmt)__VA_OPT__(, ) __VA_ARGS__)

#endif /* !ANDY_ERROR_H */
