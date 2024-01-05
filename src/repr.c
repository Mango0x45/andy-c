#include <stdio.h>

#include "lexer.h"

#define btoa(x) ((x) ? "true" : "false")
#define eiprintf(d, ...) \
	do { \
		indent(d); \
		fprintf(stderr, __VA_ARGS__); \
	} while (false)

static void
indent(int d)
{
	for (int i = 0; i < d; i++)
		fputs("    ", stderr);
}

void
repr_lextok(struct lextok lt, int d)
{
	eiprintf(d, "lextok {\n");
	eiprintf(d + 1, ".p    = ‘%.*s’\n", (int)lt.len, lt.p);
	eiprintf(d + 1, ".len  = %zu\n", lt.len);
	eiprintf(d, "}");
	if (lt.kind == LTK_NL && lt.len == 0)
		fprintf(stderr, " /* EOF */");
	fputc('\n', stderr);
}

void
repr_lex_rdr_flags(struct lex_rdr_flags f, int d)
{
	eiprintf(d, "lex_rdr_flags {\n");
	eiprintf(d + 1, ".app = %s\n", btoa(f.app));
	eiprintf(d + 1, ".clb = %s\n", btoa(f.clb));
	eiprintf(d + 1, ".fd  = %s\n", btoa(f.fd));
	eiprintf(d + 1, ".rd  = %s\n", btoa(f.rd));
	eiprintf(d + 1, ".wr  = %s\n", btoa(f.wr));
	eiprintf(d, "}\n");
}

void
repr_lex_var_flags(struct lex_var_flags f, int d)
{
	eiprintf(d, "lex_var_flags {\n");
	eiprintf(d + 1, ".cc  = %s\n", btoa(f.cc));
	eiprintf(d + 1, ".len = %s\n", btoa(f.len));
	eiprintf(d, "}\n");
}
