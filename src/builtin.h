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
builtin builtin_cd, builtin_echo, builtin_false, builtin_get, builtin_set,
	builtin_true, builtin_umask;

#endif /* !ANDY_BUILTIN_H */
