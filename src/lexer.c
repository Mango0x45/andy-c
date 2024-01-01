#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#include "da.h"
#include "lexer.h"
#include "uni.h"
#include "utf8.h"

typedef enum {
	LS_BRACE, /* In braces */
	LS_PAREN, /* In parenthesis */
} lexstate_t;

struct lexstates {
	lexstate_t *buf;
	size_t len, cap;
};

static char8_t *lexarg(const char8_t *, struct lexstates *);
static bool in_comment(rune_t);
static bool is_arg_char(rune_t);

static const bool metachars[CHAR_MAX + 1] = {
	['#'] = true, ['('] = true, [';'] = true, ['<'] = true,
	['>'] = true, ['{'] = true, ['|'] = true,
};

void
lexstr(const char8_t *s, struct lextoks *toks)
{
	struct lexstates ls;

	dainit(&ls, 8);
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

		/* Set tok to the token of kind k and byte-length w */
#define TOKLIT(w, k) \
	do { \
		tok.p = s; \
		tok.len = w; \
		tok.kind = k; \
		s += w; \
	} while (false)

		/* Assert if we are at the string-literal x */
#define ISLIT(x) (!strncmp((char *)s, (x), sizeof(x) - 1))
		if (ch == '|') {
			TOKLIT(1, LTK_PIPE);
		} else if (ch == ';' || ch == '\n') {
			TOKLIT(1, LTK_NL);
		} else if (ch == '{') {
			TOKLIT(1, LTK_BRC_O);
			dapush(&ls, LS_BRACE);
		} else if (ch == '(') {
			TOKLIT(1, LTK_PRN_O);
			dapush(&ls, LS_PAREN);
		} else if (ch == '}' && datopis(&ls, LS_BRACE)) {
			TOKLIT(1, LTK_BRC_C);
			ls.len--;
		} else if (ch == ')' && datopis(&ls, LS_PAREN)) {
			TOKLIT(1, LTK_PRN_C);
			ls.len--;
		} else if (ISLIT(">>")) {
			TOKLIT(2, LTK_RDR_APP);
		} else if (ISLIT(">!")) {
			TOKLIT(2, LTK_RDR_CLB);
		} else if (ch == '>') {
			TOKLIT(1, LTK_RDR_RD);
		} else if (ch == '<') {
			TOKLIT(1, LTK_RDR_WR);
		} else {
			tok.p = s;
			s = lexarg(s, &ls);
			tok.len = s - tok.p;
			tok.kind = LTK_ARG;
		}
#undef ISLIT
#undef TOKLIT

		dapush(toks, tok);
	}

	dapush(toks, ((struct lextok){.kind = LTK_NL}));
	free(ls.buf);
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
lexarg(const char8_t *s, struct lexstates *ls)
{
	rune_t ch;
	size_t i = 0;

	while ((ch = utf8iter(s, &i))) {
		if (!is_arg_char(ch))
			break;
		if (ch == '}' && datopis(ls, LS_BRACE))
			break;
		if (ch == ')' && datopis(ls, LS_PAREN))
			break;
	}

	return (char8_t *)s + i - utf8wdth(ch);
}
