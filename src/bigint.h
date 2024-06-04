#ifndef ANDY_BIGINT_H
#define ANDY_BIGINT_H

#include <mbstring.h>

bool isbigint(struct u8view);
int bigintcmp(const void *, const void *);

#endif /* !ANDY_BIGINT_H */
