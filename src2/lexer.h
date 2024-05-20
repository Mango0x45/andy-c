#ifndef ANDY_LEXER_H
#define ANDY_LEXER_H

#include <dynarr.h>
#include <mbstring.h>

enum lex_tok_kind {
	LTK_ARG, /* Argument */
	LTK_NL,  /* End of line */
};

struct lextok {
	enum lex_tok_kind kind;
	struct u8view sv;
};

typedef dynarr(struct lextok) lextoks;

/* Test if a rune is horizontal whitespace */
[[unsequenced, nodiscard]] bool rishws(rune);

/* Test if a rune is vertical whitespace */
[[unsequenced, nodiscard]] bool risvws(rune);

/* Lex the string SV and store the resulting tokens in TOKS.  The input filename
   F is used for diagnostic messages. */
[[gnu::nonnull]] bool lexstr(const char *f, struct u8view sv, lextoks *toks);

#endif /* !ANDY_LEXER_H */
