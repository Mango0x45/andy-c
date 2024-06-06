#ifndef ANDY_EXEC_H
#define ANDY_EXEC_H

#include <alloc.h>

#include "parser.h"
#include "symtab.h"

constexpr int FDCNT = 3;

struct ctx {
	int fds[FDCNT];
	arena *a;
};

void shellinit(void);
int execprog(struct program, struct ctx);

extern struct symtab symboltable;

#endif /* !ANDY_EXEC_H */
