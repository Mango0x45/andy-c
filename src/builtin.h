#ifndef ANDY_BUILTIN_H
#define ANDY_BUILTIN_H

#include <stddef.h>

#include "exec.h"

typedef int (*builtin_fn)(char **, size_t, struct ctx);

struct lookup {
	char *name;
	builtin_fn fn;
};

builtin_fn lookup_builtin(const char *);

int builtin_cd(char **, size_t, struct ctx);
int builtin_echo(char **, size_t, struct ctx);
int builtin_false(char **, size_t, struct ctx);
int builtin_get(char **, size_t, struct ctx);
int builtin_set(char **, size_t, struct ctx);
int builtin_true(char **, size_t, struct ctx);

#endif /* !ANDY_BUILTIN_H */
