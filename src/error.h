#ifndef ANDY_ERROR_H
#define ANDY_ERROR_H

#include <mbstring.h>

void errinit(void);
void erremit(const char *, const char8_t *, struct u8view, size_t, const char *,
             ...);

#endif /* !ANDY_ERROR_H */
