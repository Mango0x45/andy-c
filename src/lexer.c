#include <err.h>
#include <langinfo.h>
#include <stdlib.h>
#include <string.h>
#include <uchar.h>

#include "da.h"
#include "lexer.h"
#include "utf8.h"
#include "util.h"

#define SPECIAL WHITESPACE u8"\n\"#$&'();<>{}|‘“"

struct lexstates {
	enum {
		LS_BRACE,
		LS_PAREN,
	} *buf;
	size_t len, cap;
};

[[nonnull]] static void warn_unterminated(const char8_t *, const char8_t *,
                                          size_t, const char *);

void
lexstr(const char *file, const char8_t *s, struct lextoks *toks)
{
	rune ᚱ;
	const char8_t *p, *bgn = s;
	struct lexstates ls;

	dainit(&ls, 8);
	dainit(toks, 64);

	if ((p = c8chk(s))) {
		warnx("%s: invalid UTF-8 sequence beginning with ‘0x%2X’", file, *p);
		return;
	}

	/* TODO: Remove cast once Clangd gets u8 string literal support */
	while (*(s = c8pcbrknul(s, (char8_t *)WHITESPACE))) {
		struct lextok tok = {};

		/* Set tok to the token of kind k and byte-length w */
#define TOKLIT(w, k) \
	do { \
		tok.p = s; \
		tok.kind = (k); \
		tok.len = (w); \
		s += (w); \
	} while (false)

		/* Assert if we are at the string-literal x */
#define ISLIT(x) (!strncmp((char *)s, (x), sizeof(x) - 1))

		ᚱ = c8tor(s);
		if (ᚱ == '#') {
			if (!*(s = c8chrnul(s, '\n')))
				break;
			TOKLIT(1, LTK_NL);
		} else if (ISLIT("&&")) {
			TOKLIT(2, LTK_LAND);
		} else if (ISLIT("||")) {
			TOKLIT(2, LTK_LOR);
		} else if (ᚱ == '|') {
			TOKLIT(1, LTK_PIPE);
		} else if (ᚱ == ';' || ᚱ == '\n') {
			TOKLIT(1, LTK_NL);
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
				warn_unterminated(bgn, tok.p - 1, s - tok.p, file);
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
					warn_unterminated(bgn, tok.p - n * rwdth(U'‘'), s - tok.p,
					                  file);
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
				warn_unterminated(bgn, tok.p - n * rwdth(U'“'), tok.len, file);
				return;
			}
			s += n;
		} else if (ᚱ == '"') {
			tok.kind = LTK_STR;
			tok.p = ++s;

			/* It’s safe to treat input as ASCII here */
			for (; *s != '"'; s++) {
				if (!*s) {
					warn_unterminated(bgn, tok.p - 1, s - tok.p, file);
					return;
				}
				if (*s == '\\')
					s++;
			}

			tok.len = s - tok.p;
			s++;
		} else if (ᚱ == '$') {
			tok.kind = LTK_VAR;
			tok.p = s++;

			if (*s == '#' || *s == '^') {
				if (*s == '^')
					tok.varf.cc = true;
				else if (*s == '#')
					tok.varf.len = true;
				s++;
			}

			/* Not a valid identifier; treat it like an argument */
			if (!risstart(ᚱ = c8tor(s))) {
				s = tok.p;
				tok.varf.cc = tok.varf.len = false;
				goto lex_arg;
			}

			do
				s = c8fwd(s);
			while (riscont(ᚱ = c8tor(s)));
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
				    || (ᚱ == '&' && s[1] != '&') || (ᚱ == '|' && s[1] != '|')
				    || (ᚱ == '$' && !risstart(c8tor(c8fwd(s)))))
				{
					s = c8fwd(s);
					continue;
				}
				break;
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
warn_unterminated(const char8_t *bgn, const char8_t *s, size_t n,
                  const char *file)
{
	size_t row, col;
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

	row = 1;
	col = 0;

	for (rune ᚱ; bgn < s && (ᚱ = c8tor(bgn)); bgn = c8fwd(bgn)) {
		if (ᚱ == '\n') {
			col = 0;
			row++;
		} else
			col++;
	}

	warnx("%s:%zu:%zu: Unterminated string %s%.*s%s%s", file, row, col, oquo,
	      (int)min(n, 12), s, n > 12 ? elip : "", cquo);
}
