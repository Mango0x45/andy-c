#include <limits.h>
#include <stddef.h>

#include "da.h"
#include "lexer.h"
#include "uni.h"
#include "utf8.h"

static bool in_comment(rune_t);
static bool is_arg_char(rune_t);

static const bool metachars[CHAR_MAX + 1] = {
	['|'] = true,
	[';'] = true,
};

void
lexstr(const char *s, struct lextoks *toks)
{
	dainit(toks, 64);

	while (*s) {
		rune_t ch;
		size_t i = 0;
		struct lextok tok;

		s = utf8skipf(s, unispace);
		ch = utf8iter(s, &i);

		/* Comments */
		if (ch == '#') {
			s = utf8skipf(s, in_comment);
			continue;
		}

		if (ch == '|') {
			tok.p = s;
			tok.len = 1;
			tok.kind = LTK_PIPE;
			s++;
		} else if (ch == ';' || ch == '\n') {
			tok.p = s;
			tok.len = 1;
			tok.kind = LTK_NL;
			s++;
		} else {
			tok.p = s;
			s = utf8skipf(s, is_arg_char);
			tok.len = s - tok.p;
			tok.kind = LTK_ARG;
		}

		dapush(toks, tok);
	}
}

bool
in_comment(rune_t ch)
{
	return ch != '\n' && ch != '\0';
}

bool
is_arg_char(rune_t ch)
{
	if (ch <= CHAR_MAX && metachars[ch])
		return false;
	return !unispace(ch);
}
