#include <stdio.h>
#include <uchar.h>

#include "lexer.h"
#include "repr.h"
#include "utf8.h"
#include "util.h"

#define btoa(x)      ((x) ? "true" : "false")
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define eiprintf(d, ...) \
	do { \
		indent(d); \
		eprintf(__VA_ARGS__); \
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
	size_t n = 0;

	eprintf("lextok {\n");

	eiprintf(d + 1, ".p    = ");
	for (const char8_t *s = lt.p; (size_t)(s - lt.p) < lt.len; s = c8fwd(s)) {
		rune ᚱ = c8tor(s);
		if (ᚱ == U'’') {
			size_t m = c8rspn(s, ᚱ);
			s += m;
			n = max(n, m / rwdth(ᚱ));
		}
	}
	n++;
	for (size_t i = 0; i < n; i++)
		eprintf("‘");
	eprintf("%.*s", (int)lt.len, lt.p);
	for (size_t i = 0; i < n; i++)
		eprintf("’");
	eprintf("\n");

	eiprintf(d + 1, ".len  = %zu\n", lt.len);
	eiprintf(d + 1, ".kind = ");
	_repr(d + 1, lt.kind);

	switch (lt.kind) {
	case LTK_RDR:
		eiprintf(d + 1, ".rf   = ");
		_repr(d + 1, lt.rf);
		break;
	case LTK_VAR:
		eiprintf(d + 1, ".vf   = ");
		_repr(d + 1, lt.vf);
		break;
	default:
		/* Silence warnings for unmatched cases */
	}

	eiprintf(d, "}");
	if (lt.kind == LTK_NL && lt.len == 0)
		eprintf(" /* EOF */");
	eprintf("\n");
}

void
repr_lex_token_kind(lex_token_kind_t k, [[maybe_unused]] int _)
{
	switch (k) {
	case LTK_ARG:
		eprintf("ARG\n");
		break;
	case LTK_BRC_C:
		eprintf("BRC_C\n");
		break;
	case LTK_BRC_O:
		eprintf("BRC_O\n");
		break;
	case LTK_NL:
		eprintf("NL\n");
		break;
	case LTK_PIPE:
		eprintf("PIPE\n");
		break;
	case LTK_PRC_RD:
		eprintf("RD\n");
		break;
	case LTK_PRC_RDWR:
		eprintf("RDWR\n");
		break;
	case LTK_PRC_SUB:
		eprintf("PRC_SUB\n");
		break;
	case LTK_PRC_WR:
		eprintf("PRC_WR\n");
		break;
	case LTK_PRN_C:
		eprintf("PRN_C\n");
		break;
	case LTK_PRN_O:
		eprintf("PRN_O\n");
		break;
	case LTK_RDR:
		eprintf("RDR\n");
		break;
	case LTK_STR_RAW:
		eprintf("STR_RAW\n");
		break;
	case LTK_STR:
		eprintf("STR\n");
		break;
	case LTK_VAR:
		eprintf("VAR\n");
		break;
	default:
		unreachable();
	}
}

void
repr_lex_rdr_flags(struct lex_rdr_flags f, int d)
{
	eprintf("lex_rdr_flags {\n");
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
	eprintf("lex_var_flags {\n");
	eiprintf(d + 1, ".cc  = %s\n", btoa(f.cc));
	eiprintf(d + 1, ".len = %s\n", btoa(f.len));
	eiprintf(d, "}\n");
}
