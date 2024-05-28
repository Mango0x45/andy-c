#include <string.h>

#include <bitset.h>
#include <dynarr.h>
#include <errors.h>
#include <macros.h>
#include <mbstring.h>
#include <rune.h>
#include <unicode/prop.h>
#include <unicode/string.h>

#include "lexer.h"
#include "syntax.h"

#define report(...)                                                            \
	do {                                                                       \
		warn(__VA_ARGS__);                                                     \
		tok.sv.len = l->sv.p - tok.sv.p;                                       \
		tok.kind = -1;                                                         \
		longjmp(*l->err, 1);                                                   \
	} while (false)

#define ISLIT(y) (strncmp(tok.sv.p, (y), sizeof(y) - 1) == 0)

/* Set tok to the token of kind K which has the string value of S */
#define TOKLIT(s, k)                                                           \
	do {                                                                       \
		tok.kind = (k);                                                        \
		tok.sv = U8(s);                                                        \
	} while (false)

/* Test if a rune is valid in an argument */
[[unsequenced, nodiscard]] static bool risarg(rune);

struct lextok
lexnext(struct lexer *l)
{
	if (l->next.exists) {
		l->next.exists = false;
		return l->cur = l->next.t;
	}

	int w;
	for (rune ch; (w = ucsnext(&ch, &l->sv)) != 0;) {
		if (rishws(ch))
			continue;
		struct lextok tok = {.sv.p = l->sv.p - w};

		if (ch == '#') {
			do
				w = ucsnext(&ch, &l->sv);
			while (w > 0 && !risvws(ch));
			tok.kind = LTK_NL;
		} else if (ch == ';') {
			tok.sv.len = 1;
			tok.kind = LTK_SEMI;
		} else if (ISLIT("&&")) {
			TOKLIT("&&", LTK_LAND);
			VSHFT(&l->sv, 1);
		} else if (ISLIT("||")) {
			TOKLIT("||", LTK_LOR);
			VSHFT(&l->sv, 1);
		} else if (ch == '|') {
			tok.sv.len = 1;
			tok.kind = LTK_PIPE;
		} else {
			size_t n = 0;
			VSHFT(&l->sv, -w);
			do {
				char8_t e;
				n += w = ucsnext(&ch, &l->sv);

				if (w > 0 && ch == '\\') {
					n += w = ucsnext(&ch, &l->sv);
					if (w == 0) {
						report("%s:%tu: expected escape sequence but got end "
						       "of file",
						       l->file, l->sv.p - l->base - 1);
					} else if ((e = escape(ch, true)) == 0) {
						VSHFT(&l->sv, -w);
						struct u8view g, cpy = l->sv;
						ucsgnext(&g, &cpy);
						report("%s:%tu: invalid escape sequence ‘\\%.*s’",
						       l->file, l->sv.p - l->base - 1, SV_PRI_ARGS(g));
					}
				}
			} while (w > 0 && risarg(ch));
			if (w > 0) {
				n -= w;
				VSHFT(&l->sv, -w);
			}

			if ((tok.sv.len = n) == 0)
				break;
			tok.kind = LTK_WORD;
		}

		return l->cur = tok;
	}

	return l->cur = (struct lextok){
		.kind = LTK_EOF,
		.sv.p = l->sv.p,
	};
}

struct lextok
lexpeek(struct lexer *l)
{
	if (!l->next.exists) {
		l->next.t = lexnext(l);
		l->next.exists = true;
	}
	return l->next.t;
}

bool
risarg(rune ch)
{
	return !uprop_is_pat_ws(ch) && !rismeta(ch);
}
