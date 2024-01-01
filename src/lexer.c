#include <limits.h>
#include <uchar.h>

#include "da.h"
#include "lexer.h"
#include "uni.h"
#include "utf8.h"

static char8_t *lexarg(const char8_t *);
static bool in_comment(rune_t);
static bool is_arg_char(rune_t);

static const bool metachars[CHAR_MAX + 1] = {
	['#'] = true,
	['('] = true,
	[';'] = true,
	['{'] = true,
	['|'] = true,
};

void
lexstr(const char8_t *s, struct lextoks *toks)
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

#define TOKLIT(w, k) \
	do { \
		tok.p = s; \
		tok.len = w; \
		tok.kind = k; \
		s++; \
	} while (false)

		if (ch == '|')
			TOKLIT(1, LTK_PIPE);
		else if (ch == ';' || ch == '\n')
			TOKLIT(1, LTK_NL);
		else if (ch == '(')
			TOKLIT(1, LTK_PRN_O);
		else if (ch == ')')
			TOKLIT(1, LTK_PRN_C);
		else if (ch == '{')
			TOKLIT(1, LTK_BRC_O);
		else if (ch == '}')
			TOKLIT(1, LTK_BRC_C);
		else {
			tok.p = s;
			s = lexarg(s);
			tok.len = s - tok.p;
			tok.kind = LTK_ARG;
		}

#undef TOKLIT

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

char8_t *
lexarg(const char8_t *s)
{
	rune_t ch;
	size_t i = 0;

	while ((ch = utf8iter(s, &i))) {
		if (!is_arg_char(ch))
			break;
	}

	return (char8_t *)s + i - utf8wdth(ch);
}
