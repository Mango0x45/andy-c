#ifndef ANDY_SYMTAB_H
#define ANDY_SYMTAB_H

#include <mbstring.h>

#include "vartab.h"

struct symtabpair {
	struct u8view k;
	struct vartab *v;
};

struct symtab {
	struct symtabbkt {
		dafields(struct symtabpair)
	} *bkts;
	size_t len, cap;
};

struct symtab mksymtab(void);
struct vartab *symtabget(struct symtab, struct u8view);
void symtabadd(struct symtab *, struct u8view, struct vartab *);
void symtabdel(struct symtab *, struct u8view);
void symtabfree(struct symtab);

#endif /* !ANDY_SYMTAB_H */
