/* Apple doesn’t have uchar.h even though it’s C11… fml */

#ifndef ANDY_UCHAR_H
#define ANDY_UCHAR_H

#include <stddef.h> /* For size_t */
#include <stdint.h>

typedef unsigned char char8_t;
typedef uint_least32_t char32_t;

#endif
