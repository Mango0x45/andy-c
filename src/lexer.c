#include <err.h>
#include <langinfo.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#include "da.h"
#include "lexer.h"
#include "utf8.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

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
static void warn_unterminated(const char8_t *, size_t, const char *,
                              struct lexpos);

static const char8_t metachars[] = u8"\"#'(;<>{|‘“";

void
lexstr(const char *file, const char8_t *s, struct lextoks *toks)
{
	rune ᚱ;
	char8_t *p;
	struct lexstates ls;
	struct lexpos lp = {0, 1};

	dainit(&ls, 8);
	dainit(toks, 64);

	if ((p = c8chk(s))) {
		warnx("%s: invalid UTF-8 sequence beginning with ‘0x%2X’", file, *p);
		return;
	}

	while (*(s = c8pcbrknul(s, WHITESPACE))) {
		struct lextok tok = {};

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

		ᚱ = c8tor(s);
		if (ᚱ == '#') {
			if (!*(s = c8chrnul(s, '\n')))
				break;
			lexpfw('\n', &lp);
			TOKLIT(1, LTK_NL);
		} else if (ᚱ == '|') {
			TOKLIT(1, LTK_PIPE);
		} else if (ᚱ == ';') {
			TOKLIT(1, LTK_NL);
		} else if (ᚱ == '\n') {
			TOKLIT(1, LTK_NL);
			lexpfw('\n', &lp);
		} else if (ᚱ == '{') {
			TOKLIT(1, LTK_BRC_O);
			dapush(&ls, LS_BRACE);
		} else if (ᚱ == '(') {
			TOKLIT(1, LTK_PRN_O);
			dapush(&ls, LS_PAREN);
		} else if (ᚱ == '}' && datopis(&ls, LS_BRACE)) {
			TOKLIT(1, LTK_BRC_C);
			ls.len--;
		} else if (ᚱ == ')' && datopis(&ls, LS_PAREN)) {
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
		} else if (ᚱ == '>') {
			TOKLIT(1, LTK_RDR_RD);
			if (*s == '&') {
				tok.flags = LF_FD;
				s++;
			}
		} else if (ᚱ == '<') {
			TOKLIT(1, LTK_RDR_WR);
			if (*s == '&') {
				tok.flags = LF_FD;
				s++;
			}
		} else if (ᚱ == '\'') {
			tok.kind = LTK_STR_RAW;
			tok.p = ++s;
			if (!*(s = c8chrnul(s, ᚱ))) {
				warn_unterminated(tok.p, s - tok.p, file, lp);
				return;
			}
			tok.len = s - tok.p;
			s++;
		} else if (ᚱ == U'‘') {
			size_t n;

			tok.kind = LTK_STR_RAW;
			tok.p = s += n = c8rspn(s, U'‘');

			for (;;) {
				size_t m;
				if (!*(s = c8chrnul(s, U'’'))) {
					warn_unterminated(tok.p, s - tok.p, file, lp);
					return;
				}
				if ((m = c8nrspn(s, U'’', n)) == n)
					break;
				s += m;
			}

			tok.len = s - tok.p;
			s += n;
		} else if (ᚱ == U'“') {
			size_t n;

			tok.kind = LTK_STR_RAW;
			tok.p = s += n = c8rspn(s, ᚱ);

			while ((ᚱ = c8tor(s))) {
				if (ᚱ == '\\')
					s++;
				else if (ᚱ == U'”') {
					size_t m = c8nrspn(s, U'”', n);
					if (n == m)
						break;
					s += m;
				}
				s = c8fwd(s);
			}

			tok.len = s - tok.p;
			if (!ᚱ) {
				warn_unterminated(tok.p, tok.len, file, lp);
				return;
			}
			s += n;
		} else if (ᚱ == '"') {
			tok.kind = LTK_STR;
			tok.p = ++s;

			/* It’s safe to treat input as ASCII here */
			for (; *s != '"'; s++) {
				if (!*s) {
					warn_unterminated(tok.p, s - tok.p, file, lp);
					return;
				}
				if (*s == '\\')
					s++;
			}

			tok.len = s - tok.p;
			s++;
		} else {
			tok.kind = LTK_ARG;
			tok.p = s;

			while ((ᚱ = c8tor(s))) {
				if (*c8chrnul(metachars, ᚱ) || risblank(ᚱ))
					break;
				if (ᚱ == '}' && datopis(&ls, LS_BRACE))
					break;
				if (ᚱ == ')' && datopis(&ls, LS_PAREN))
					break;
				s = c8fwd(s);
				lexpfw(ᚱ, &lp);
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
		lp->col += rwdth(ch);
}

void
warn_unterminated(const char8_t *s, size_t n, const char *file,
                  struct lexpos lp)
{
	char *oquo, *cquo, *elip;
	char *cs = nl_langinfo(CODESET);

	if (strcmp(cs, "UTF-8") == 0) {
		oquo = (char *)u8"‘";
		cquo = (char *)u8"’";
		elip = (char *)u8"…";
	} else {
		oquo = "`";
		cquo = "'";
		elip = "..";
	}

	warnx("%s:%zu:%zu: Unterminated string %s%.*s%s%s", file, lp.row, lp.col,
	      oquo, (int)min(n, 12), s, n > 12 ? elip : "", cquo);
}
