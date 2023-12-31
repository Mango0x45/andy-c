#include <stdio.h>

#include "da.h"
#include "lexer.h"
#include "utf8.h"

void
lexstr(const char *s, struct lextoks *toks)
{
	(void)toks;

	size_t i = 0;
	while (s[i]) {
		rune_t r = utf8iter(s, &i);
		if (r == UNI_REPL_CHAR) {
			puts("Invalid char");
			break;
		}
		printf("Got rune ‘%lc’\n", r);
	}
}
