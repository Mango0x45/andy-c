#ifndef ANDY_LEXER_H
#define ANDY_LEXER_H

#include <uchar.h>

typedef enum {
	LTK_ARG,     /* Argument */
	LTK_BKT_C,   /* Closing bracket */
	LTK_BKT_O,   /* Opening bracket */
	LTK_BRC_C,   /* Closing brace */
	LTK_BRC_O,   /* Opening brace */
	LTK_NL,      /* End of statement */
	LTK_PCR,     /* Process redirection */
	LTK_PCS,     /* Process substitution */
	LTK_PIPE,    /* Pipe */
	LTK_PRN_C,   /* Closing parenthesis */
	LTK_PRN_O,   /* Opening parenthesis */
	LTK_RDR,     /* Redirection */
	LTK_STR_RAW, /* Raw string */
	LTK_STR,     /* String */
	LTK_VAR,     /* Variable reference */
} lex_token_kind_t;

struct lex_pcr_flags {
	bool rd : 1; /* Read */
	bool wr : 1; /* Write */
};

struct lex_rdr_flags {
	bool app : 1; /* Append */
	bool clb : 1; /* Clobber */
	bool fd  : 1; /* ‘&’ suffix */
	bool rd  : 1; /* Read */
	bool wr  : 1; /* Write */
};

struct lex_var_flags {
	bool cc  : 1; /* ‘^’ concatination flag */
	bool len : 1; /* ‘#’ length flag */
};

struct lextok {
	const char8_t *p;
	size_t len;
	union {
		struct lex_pcr_flags pcrf;
		struct lex_rdr_flags rdrf;
		struct lex_var_flags varf;
	};
	lex_token_kind_t kind;
};

struct lextoks {
	struct lextok *buf;
	size_t len, cap;
};

/* Lex the code in the UTF-8-encoded string s and store the resulting tokens in
   toks.  The input filename f is used for diagnostic messages. */
void lexstr(const char *f, const char8_t *s, struct lextoks *toks);

#endif /* !ANDY_LEXER_H */
