#include <stdio.h>

#include "repr.h"

#define btoa(x)      ((x) ? "true" : "false")
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define eiprintf(d, ...)                                                       \
	do {                                                                       \
		indent(d);                                                             \
		eprintf(__VA_ARGS__);                                                  \
	} while (false)

static void
indent(int d)
{
	for (int i = 0; i < d; i++)
		eprintf("    ");
}

void
repr_lextok(struct lextok lt, int d)
{
	eprintf("lextok {\n");
	eiprintf(d + 1, ".kind = ");
	_repr(d + 1, lt.kind);
	if (lt.kind == LTK_NL || lt.kind == LTK_EOF)
		eiprintf(d + 1, u8".sv   = /* … */\n");
	else
		eiprintf(d + 1, u8".sv   = ‘%.*s’\n", SV_PRI_ARGS(lt.sv));
	eiprintf(d, "}\n");
}

void
repr_lex_tok_kind(enum lex_tok_kind k, int)
{
	switch (k) {
	case LTK_ARG:
		eprintf("ARG\n");
		break;
	case LTK_EOF:
		eprintf("EOF\n");
		break;
	case LTK_ERR:
		eprintf("ERR\n");
		break;
	case LTK_LAND:
		eprintf("LAND\n");
		break;
	case LTK_LOR:
		eprintf("LOR\n");
		break;
	case LTK_NL:
		eprintf("NL\n");
		break;
	case LTK_SEMI:
		eprintf("SEMI\n");
		break;
	case _LTK_TERM:
		break;
	}
}
