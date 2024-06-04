#ifndef ANDY_EXEC_H
#define ANDY_EXEC_H

#include <alloc.h>

#include "parser.h"
#include "symtab.h"

struct ctx {
	arena *a;
};

void shellinit(void);
int exec_prog(struct program, struct ctx);

extern struct symtab symboltable;

#endif /* !ANDY_EXEC_H */
