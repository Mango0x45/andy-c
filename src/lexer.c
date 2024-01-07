#include <err.h>
#include <langinfo.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#include "da.h"
#include "lexer.h"
#include "utf8.h"
#include "util.h"

#define SPECIAL WHITESPACE u8"\n\"#$'();<>{}|‘“"

struct lexstates {
	enum {
		LS_BRACE,
		LS_PAREN,
	} *buf;
	size_t len, cap;
};

struct lexpos {
	size_t col; /* 0-based */
	size_t row; /* 1-based */
};

static void lexpfw(rune, struct lexpos *);
static void warn_unterminated(const char8_t *, size_t, const char *,
                              struct lexpos);

void
lexstr(const char *file, const char8_t *s, struct lextoks *toks)
{
	rune ᚱ;
	const char8_t *p;
	struct lexstates ls;
	struct lexpos lp = {0, 1};

	dainit(&ls, 8);
	dainit(toks, 64);

	if ((p = c8chk(s))) {
		warnx("%s: invalid UTF-8 sequence beginning with ‘0x%2X’", file, *p);
		return;
	}

	/* TODO: Remove cast once Clangd gets u8 string literal support */
	while (p = s, *(s = c8pcbrknul(p, (char8_t *)WHITESPACE))) {
		struct lextok tok = {};
		lp.col += s - p;

		/* Set tok to the token of kind k and byte-length w */
#define TOKLIT(w, k) \
	do { \
		tok.p = s; \
		tok.kind = (k); \
		tok.len = (w); \
		lp.col += (w); \
		s += (w); \
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
			TOKLIT(2, LTK_PCS);
			dapush(&ls, LS_BRACE);
		} else if (ISLIT("<{")) {
			TOKLIT(2, LTK_PCR);
			tok.pcrf.rd = true;
			dapush(&ls, LS_BRACE);
		} else if (ISLIT(">{")) {
			TOKLIT(2, LTK_PCR);
			tok.pcrf.wr = true;
			dapush(&ls, LS_BRACE);
		} else if (ISLIT("<>{")) {
			TOKLIT(3, LTK_PCR);
			tok.pcrf.rd = tok.pcrf.wr = true;
			dapush(&ls, LS_BRACE);

#define CHKFLAG(c, f) \
	do { \
		if (*s == (c)) { \
			tok.rdrf.f = true; \
			tok.len++; \
			lp.col++; \
			s++; \
		} \
	} while (false)

		} else if (ISLIT(">>")) {
			TOKLIT(2, LTK_RDR);
			CHKFLAG('&', fd);
			tok.rdrf.app = true;
			tok.rdrf.clb = true;
		} else if (ᚱ == '>') {
			TOKLIT(1, LTK_RDR);
			CHKFLAG('!', clb);
			CHKFLAG('&', fd);
			tok.rdrf.wr = true;
		} else if (ᚱ == '<') {
			TOKLIT(1, LTK_RDR);
			CHKFLAG('&', fd);
			tok.rdrf.rd = true;

#undef CHKFLAG

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
		} else if (ᚱ == '$') {
			lp.col++;
			tok.kind = LTK_VAR;
			tok.p = s++;

			if (*s == '#' || *s == '^') {
				lp.col++;
				if (*s == '^')
					tok.varf.cc = true;
				else if (*s == '#')
					tok.varf.len = true;
				s++;
			}

			/* Not a valid identifier; treat it like an argument */
			if (!risstart(ᚱ = c8tor(s))) {
				lp.col -= s - tok.p;
				s = tok.p;
				tok.varf.cc = tok.varf.len = false;
				goto lex_arg;
			}

			do {
				lexpfw(ᚱ, &lp);
				s = c8fwd(s);
			} while (riscont(ᚱ = c8tor(s)));
			tok.len += s - tok.p;
		} else {
lex_arg:
			tok.kind = LTK_ARG;
			tok.p = s;

			/* TODO: Remove cast once Clangd gets u8 string literal support */
			while (*(s = c8pbrknul(s, (char8_t *)SPECIAL))) {
				ᚱ = c8tor(s);
				if ((ᚱ == '}' && !datopis(&ls, LS_BRACE))
				    || (ᚱ == ')' && !datopis(&ls, LS_PAREN))
				    || (ᚱ == '$' && !risstart(c8tor(c8fwd(s)))))
				{
					s = c8fwd(s);
					continue;
				}
				break;
			}

			tok.len = s - tok.p;
			if (ᚱ == '\n')
				lexpfw('\n', &lp);
			else
				lp.col += tok.len;
		}

#undef ISLIT
#undef TOKLIT

		dapush(toks, tok);
	}

	dapush(toks, ((struct lextok){.kind = LTK_NL}));
	free(ls.buf);
}

void
lexpfw(rune ᚱ, struct lexpos *lp)
{
	if (ᚱ == '\n') {
		lp->col = 0;
		lp->row++;
	} else
		lp->col += rwdth(ᚱ);
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
