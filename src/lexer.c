#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#include "da.h"
#include "lexer.h"
#include "utf8.h"

typedef enum {
	LS_BRACE, /* In braces */
	LS_PAREN, /* In parenthesis */
} lexstate_t;

struct lexstates {
	lexstate_t *buf;
	size_t len, cap;
};

struct lexpos {
	size_t col; /* 0-based */
	size_t row; /* 1-based */
};

static void lexpfw(rune, struct lexpos *);

static const char8_t metachars[] = u8"\"#'(;<>{|‘“";

void
lexstr(const char *file, const char8_t *s, struct lextoks *toks)
{
	rune ch;
	struct lexstates ls;
	struct lexpos lp = {0, 1};

	dainit(&ls, 8);
	dainit(toks, 64);

	for (const char8_t *p = s; ((ch = utf8next(&p)));) {
		if (ch == REPL_CHAR) {
			warnx("%s: invalid UTF-8 sequence near ‘0x%2X’", file, *p);
			return;
		}
	}

	while (*s) {
		struct lextok tok = {};

		while (risblank(utf8peek(s)))
			lexpfw(utf8next(&s), &lp);

			/* Set tok to the token of kind k and byte-length w */
#define TOKLIT(w, k) \
	do { \
		tok.p = s; \
		tok.kind = k; \
		tok.len = w; \
		lp.col += w; \
		s += w; \
	} while (false)

			/* Assert if we are at the string-literal x */
#define ISLIT(x) (!strncmp((char *)s, (x), sizeof(x) - 1))

		ch = utf8peek(s);
		if (ch == '#') {
			if (!(s = utf8chr(s, '\n')))
				break;
			lexpfw('\n', &lp);
		} else if (ch == '|') {
			TOKLIT(1, LTK_PIPE);
		} else if (ch == ';') {
			TOKLIT(1, LTK_NL);
		} else if (ch == '\n') {
			TOKLIT(1, LTK_NL);
			lexpfw('\n', &lp);
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
			size_t n, m;

			tok.kind = LTK_STR_RAW;
			tok.p = s += n = utf8pfx(s, ch);

			while ((m = utf8npfx(s, U'’', n)) != n) {
				s += m;
				utf8next(&s);
			}

			tok.len = s - tok.p;
			s += n;
		} else if (ch == '\'') {
			tok.kind = LTK_STR_RAW;
			tok.p = ++s;
			if (!(s = utf8chr(s, ch))) {
				warnx("%s:%zu:%zu: Unterminated string", file, lp.row, lp.col);
				return;
			}
			tok.len = s - tok.p;
			s++;
		} else if (ch == U'“') {
			size_t n;

			tok.kind = LTK_STR_RAW;
			tok.p = s += n = utf8pfx(s, ch);

			while ((ch = utf8peek(s))) {
				if (ch == '\\')
					s++;
				else if (ch == U'”') {
					size_t m = utf8npfx(s, U'”', n);
					if (n == m)
						break;
					s += m;
				}
				utf8next(&s);
			}

			tok.len = s - tok.p;
			s += n;
		} else if (ch == '"') {
			tok.kind = LTK_STR;
			tok.p = ++s;

			/* It’s safe to treat input as ASCII here */
			for (; *s != '"'; s++) {
				if (*s == '\\')
					s++;
			}

			tok.len = s - tok.p;
			s++;
		} else {
			tok.kind = LTK_ARG;
			tok.p = s;

			while ((ch = utf8peek(s))) {
				if (utf8chr(metachars, ch) || risblank(ch))
					break;
				if (ch == '}' && datopis(&ls, LS_BRACE))
					break;
				if (ch == ')' && datopis(&ls, LS_PAREN))
					break;
				utf8next(&s);
				lexpfw(ch, &lp);
			}

			tok.len = s - tok.p;
		}

#undef ISLIT
#undef TOKLIT

		dapush(toks, tok);
	}

	dapush(toks, ((struct lextok){.kind = LTK_NL}));
	free(ls.buf);
}

void
lexpfw(rune ch, struct lexpos *lp)
{
	if (ch == '\n') {
		lp->col = 0;
		lp->row++;
	} else
		lp->col += utf8wdth(ch);
}
