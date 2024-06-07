#ifndef ANDY_BUILTIN_H
#define ANDY_BUILTIN_H

#include <stddef.h>

#include "exec.h"

typedef int builtin(char **, size_t, struct ctx);

struct lookup {
	char *name;
	builtin *fn;
};

builtin *lookup_builtin(const char *);
builtin bltncd, bltnecho, bltnexec, bltnexit, bltnfalse, bltnget, bltnset,
	bltntrue, bltnumask;

#endif /* !ANDY_BUILTIN_H */
