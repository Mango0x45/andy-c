#include <err.h>
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
static bool in_comment(rune);
static bool is_arg_char(rune);

static const char8_t metachars[] = u8"\"#'(;<>{|‘“";

void
lexstr(const char8_t *s, struct lextoks *toks)
{
	rune ch;
	struct lexstates ls;

	dainit(&ls, 8);
	dainit(toks, 64);

	for (const char8_t *p = s; ((ch = utf8next(&p)));) {
		if (ch == UNI_REPL_CHAR) {
			warnx("invalid UTF-8 sequence near ‘0x%2X’", *p);
			return;
		}
	}

	while (*s) {
		struct lextok tok = {};

		s = utf8fskip(s, unispace);

		/* Set tok to the token of kind k and byte-length w */
#define TOKLIT(w, k) \
	do { \
		tok.p = (s); \
		tok.len = (w); \
		tok.kind = (k); \
		s += w; \
	} while (false)

		/* Assert if we are at the string-literal x */
#define ISLIT(x) (!strncmp((char *)s, (x), sizeof(x) - 1))

		ch = utf8peek(s);
		if (ch == '#') {
			s = utf8fskip(s, in_comment);
			continue;
		} else if (ch == '|') {
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
		} else if (ISLIT("`{")) {
			TOKLIT(2, LTK_PRC_SUB);
			dapush(&ls, LS_BRACE);
		} else if (ISLIT("<{")) {
			TOKLIT(2, LTK_PRC_RD);
			dapush(&ls, LS_BRACE);
		} else if (ISLIT(">{")) {
			TOKLIT(2, LTK_PRC_WR);
			dapush(&ls, LS_BRACE);
		} else if (ISLIT("<>{")) {
			TOKLIT(3, LTK_PRC_RDWR);
			dapush(&ls, LS_BRACE);
		} else if (ISLIT(">>")) {
			TOKLIT(2, LTK_RDR_APP);
			if (*s == '&') {
				tok.flags = LF_FD;
				s++;
			}
		} else if (ISLIT(">!")) {
			TOKLIT(2, LTK_RDR_CLB);
			if (*s == '&') {
				tok.flags = LF_FD;
				s++;
			}
		} else if (ch == '>') {
			TOKLIT(1, LTK_RDR_RD);
			if (*s == '&') {
				tok.flags = LF_FD;
				s++;
			}
		} else if (ch == '<') {
			TOKLIT(1, LTK_RDR_WR);
			if (*s == '&') {
				tok.flags = LF_FD;
				s++;
			}
		} else if (ch == U'‘') {
			size_t n = utf8pfx(s, ch);
			tok.kind = LTK_STR_RAW;
			tok.p = s += n;
			while (utf8npfx(s, U'’', n) != n)
				utf8next(&s);
			tok.len = s - tok.p;
			s += n;
		} else if (ch == '\'') {
			tok.kind = LTK_STR_RAW;
			tok.p = ++s;
			s = utf8chr(s, ch);
			tok.len = s - tok.p;
			s++;
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
in_comment(rune ch)
{
	return ch != '\n' && ch != '\0';
}

bool
is_arg_char(rune ch)
{
	return !utf8chr(metachars, ch) && !unispace(ch);
}

char8_t *
lexarg(const char8_t *s, struct lexstates *ls)
{
	rune ch;

	while ((ch = utf8next(&s))) {
		if (!is_arg_char(ch))
			break;
		if (ch == '}' && datopis(ls, LS_BRACE))
			break;
		if (ch == ')' && datopis(ls, LS_PAREN))
			break;
	}

	return (char8_t *)s - utf8wdth(ch);
}
