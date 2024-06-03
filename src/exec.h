#ifndef ANDY_EXEC_H
#define ANDY_EXEC_H

#include <alloc.h>

#include "parser.h"
#include "symtab.h"

struct ctx {
	int fds[3];
	arena *a;
};

int exec_prog(struct program, struct ctx);

extern struct symtab symboltable;

#endif /* !ANDY_EXEC_H */
