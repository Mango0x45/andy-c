#include <dynarr.h>
#include <errors.h>
#include <macros.h>
#include <mbstring.h>
#include <unicode/prop.h>

#include "lexer.h"

bool
lexstr(const char *f, struct u8view sv, lextoks *toks)
{
	const char8_t *p = ucschk(sv);
	if (p != nullptr) {
		warn("%s: invalid byte ‘%02X’ in UTF-8 sequence", f, *p);
		return false;
	}

	int w;
	for (rune ch; (w = ucsnext(&ch, &sv)) != 0;) {
		if (rishws(ch))
			continue;

		struct lextok tok = {.sv.p = sv.p - w};
		if (ch == '#') {
			do
				w = ucsnext(&ch, &sv);
			while (w > 0 && !risvws(ch));
			tok.kind = LTK_NL;
		} else {
			size_t n = w;
			do
				n += w = ucsnext(&ch, &sv);
			while (w > 0 && !uprop_is_pat_ws(ch));
			if (w > 0)
				n -= w;

			tok.sv.len = n;
			tok.kind = LTK_ARG;
		}

		DAPUSH(toks, tok);
	}

	DAPUSH(toks, ((struct lextok){.kind = LTK_NL}));
	return true;
}

bool
rishws(rune ch)
{
	static constexpr rune ws[] = {
		0x0009, /* CHARACTER TABULATION */
		0x0020, /* SPACE */
		0x200E, /* LEFT-TO-RIGHT MARK */
		0x200F, /* RIGHT-TO-LEFT MARK */
	};

	static_assert(lengthof(ws) == 4);
#pragma GCC unroll 4
	for (size_t i = 0; i < lengthof(ws); i++) {
		if (ws[i] == ch)
			return true;
	}
	return false;
}

bool
risvws(rune ch)
{
	static constexpr rune ws[] = {
		0x000A, /* LINE FEED */
		0x000B, /* LINE TABULATION */
		0x000C, /* FORM FEED */
		0x000D, /* CARRIAGE RETURN */
		0x0085, /* NEXT LINE */
		0x2028, /* LINE SEPARATOR */
		0x2029, /* PARAGRAPH SEPARATOR */
	};

	static_assert(lengthof(ws) == 7);
#pragma GCC unroll 7
	for (size_t i = 0; i < lengthof(ws); i++) {
		if (ws[i] == ch)
			return true;
	}
	return false;
}
