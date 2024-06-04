#include <setjmp.h>
#include <string.h>

#include <bitset.h>
#include <dynarr.h>
#include <errors.h>
#include <macros.h>
#include <mbstring.h>
#include <rune.h>
#include <unicode/prop.h>
#include <unicode/string.h>

#include "error.h"
#include "lexer.h"
#include "syntax.h"

#define ESEOF      "expected escape sequence but got end of file"
#define ESHEXBAD   "hexadecimal escape sequence not followed by two hex digits"
#define ESINVAL    "invalid escape sequence ‘%.*s’"
#define ESUNICLOSE "unterminated Unicode escape sequence"
#define ESUNIEMPTY "empty Unicode escape sequence"
#define ESUNIHEX   "non-hexadecimal digit ‘%.*s’ in Unicode escape sequence"
#define ESUNIOPEN  "Unicode escape sequence missing opening brace"
#define ESUNIRANGE "Unicode codepoints must be between 0–10FFFF"
#define ESUNIZERO  "leading zeros are disallowed in Unicode escape sequences"
#define EVEMPTY    "variable reference missing identifier"

#define report(hl, ...)                                                        \
	do {                                                                       \
		tok.sv.len = l->sv.p - tok.sv.p;                                       \
		erremit(l->file, l->base, hl, hl.p - l->base, __VA_ARGS__);            \
		longjmp(*l->err, 1);                                                   \
	} while (false)

#define ISLIT(y) (strncmp(tok.sv.p, (y), sizeof(y) - 1) == 0)

/* Set tok to the token of kind K which has the string value of S */
#define TOKLIT(s, k)                                                           \
	do {                                                                       \
		tok.kind = (k);                                                        \
		tok.sv.len = sizeof(s) - 1;                                            \
	} while (false)

/* Test if a rune is valid in an argument */
[[unsequenced, nodiscard]] static bool risword(rune);

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

		bool closbrkt = l->states.len != 0
		             && l->states.buf[l->states.len - 1] == LS_VAR;

		if (ch == '#') {
			do
				w = ucsnext(&ch, &l->sv);
			while (w > 0 && !risvws(ch));
			tok.kind = LTK_NL;
		} else if (risvws(ch)) {
			tok.sv.len = w;
			tok.kind = LTK_NL;
		} else if (ch == ';') {
			tok.sv.len = 1;
			tok.kind = LTK_SEMI;
		} else if (ch == '{') {
			tok.sv.len = 1;
			tok.kind = LTK_BRC_O;
		} else if (ch == '}') {
			tok.sv.len = 1;
			tok.kind = LTK_BRC_C;
		} else if (ISLIT("&&")) {
			TOKLIT("&&", LTK_LAND);
			VSHFT(&l->sv, 1);
		} else if (ISLIT("||")) {
			TOKLIT("||", LTK_LOR);
			VSHFT(&l->sv, 1);
		} else if (ch == '|') {
			tok.sv.len = 1;
			tok.kind = LTK_PIPE;
		} else if (ch == '(') {
			tok.sv.len = 1;
			tok.kind = LTK_PAR_O;
		} else if (ch == ')') {
			tok.sv.len = 1;
			tok.kind = LTK_PAR_C;
		} else if (ch == ']' && closbrkt) {
			tok.sv.len = 1;
			tok.kind = LTK_VAR_C;
			(void)DAPOP(&l->states);
		} else if (ch == '$') {
			tok.sv.len = 1;
			tok.kind = LTK_VAR;

			rune ch;
			for (int w; (w = ucsnext(&ch, &l->sv)) != 0; tok.sv.len += w) {
				if (tok.sv.len == 1 && ch == '#')
					tok.kind = LTK_VARL;
				else if (!risvar(ch)) {
					VSHFT(&l->sv, -w);
					break;
				}
			}
			if (tok.sv.len == 1)
				report(tok.sv, EVEMPTY);
			if (ch == '[') {
				tok.kind = LTK_VAR_O;
				VSHFT(&l->sv, 1);
				DAPUSH(&l->states, LS_VAR);
			}
		} else {
			size_t n = 0;
			VSHFT(&l->sv, -w);

			do {
				rune e;
				n += w = ucsnext(&ch, &l->sv);

				if (ch == ']' && closbrkt)
					break;

				if (w > 0 && ch == '\\') {
					struct u8view hl = {
						.p = l->sv.p - 1,
						.len = 1,
					};

					hl.len += w = ucsnext(&ch, &l->sv);

					if (w == 0) {
						report(hl, ESEOF);
					} else if (ch == 'x') {
						hl.len += w = ucsnext(&ch, &l->sv);
						if (w == 0 || !uprop_is_ahex(ch)) {
							hl.len -= w;
							report(hl, ESHEXBAD);
						}
						hl.len += w = ucsnext(&ch, &l->sv);
						if (w == 0 || !uprop_is_ahex(ch)) {
							hl.len -= w;
							report(hl, ESHEXBAD);
						}
					} else if (ch == 'u') {
						hl.len += w = ucsnext(&ch, &l->sv);
						if (w == 0 || ch != '{') {
							hl.len -= w;
							report(hl, ESUNIOPEN);
						}

						const char8_t *p = l->sv.p;

						for (;;) {
							hl.len += w = ucsnext(&ch, &l->sv);
							if (w == 0 || ch == '}')
								break;
							if (risvws(ch)) {
								hl.len -= w;
								report(hl, ESUNICLOSE);
							}
							if (!uprop_is_ahex(ch)) {
								VSHFT(&l->sv, -w);
								struct u8view g, cpy = l->sv;
								ucsgnext(&g, &cpy);
								hl.len += g.len - w;
								report(hl, ESUNIHEX, SV_PRI_ARGS(g));
							}
						}
						if (w == 0)
							report(hl, ESUNICLOSE);
						if (l->sv.p[-2] == '{')
							report(hl, ESUNIEMPTY);

						/* At this point we know that P is a brace-terminated
						   string */
						rune ch;
						for (const char8_t *q = p;;) {
							q += ucstor(&ch, q);
							if (ch != '0')
								break;
							report(hl, ESUNIZERO);
						}

						/* All Unicode runes must be within the range 0–10FFFF,
						   thanks to the fact that the largest rune is 1 less
						   than 110000, it’s very easy to check if we’re in the
						   range by checking only the length and first two
						   digits */
						size_t len = hl.len - sizeof("\\u{}") + 1;
						if (len > 6
						    || (len == 6
						        && (p[0] != '1'
						            || (p[0] == '1' && p[1] != '0'))))
						{
							report(hl, ESUNIRANGE);
						}
					} else if (ch == ']' && closbrkt) {
					} else if ((e = escape(ch, true)) == 0) {
						VSHFT(&l->sv, -w);
						struct u8view g, cpy = l->sv;
						ucsgnext(&g, &cpy);
						hl.len += g.len - w;
						report(hl, ESINVAL, SV_PRI_ARGS(hl));
					}

					/* If we got here, then we know that the W > 0 in the
					   do/while condition holds.  What we don’t know however is
					   if the risword(CH) condition holds, because we might have
					   been escaping a metacharacter.  As an easy way around
					   this, we can just set CH to something we know isn’t a
					   metacharacter. */
					ch = 0;
					n += hl.len;
				}
			} while (w > 0 && risword(ch));
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

	/* clang-format off */
	return l->cur = (struct lextok){
		.kind = LTK_EOF,
		.sv.p = l->sv.p,
	};
	/* clang-format on */
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
risword(rune ch)
{
	return !uprop_is_pat_ws(ch) && !rismeta(ch);
}
