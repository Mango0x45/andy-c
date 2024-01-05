#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <uchar.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "da.h"
#include "lexer.h"

static bool interactive;

static char *strtrim(char *);
static void rloop(void);

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	setlocale(LC_ALL, "");

	if ((interactive = isatty(STDOUT_FILENO)))
		rloop();

	return EXIT_SUCCESS;
}

void
rloop(void)
{
	for (;;) {
		char *p, *line;
		struct lextoks toks;

		if (!(line = readline("=> "))) {
			fputs("^D\n", stderr);
			break;
		}

		if (!*(p = strtrim(line)))
			goto empty;

		add_history(p);
		lexstr("<stdin>", (char8_t *)p, &toks);

		da_foreach (&toks, tok) {
			if (tok->kind == LTK_NL && !tok->p)
				puts("EOF");
			else {
				printf("token: ‘%.*s’", (int)tok->len, tok->p);
				if (tok->flags & LF_FD)
					printf("; flags: LS_FD");
				putchar('\n');
			}
		}

		free(toks.buf);
empty:
		free(line);
	}
}

char *
strtrim(char *s)
{
	char *e;

	while (*s == ' ' || *s == '\t')
		s++;
	for (e = s; *e; e++)
		;
	e--;
	while (e >= s && (*e == ' ' || *e == '\t'))
		e--;
	e[1] = 0;

	return s;
}
