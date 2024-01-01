#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <uchar.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "da.h"
#include "lexer.h"
#include "utf8.h"

static bool interactive;

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
		char8_t *p, *line;
		struct lextoks toks;

		if (!(line = (char8_t *)readline("=> "))) {
			fputs("^D\n", stderr);
			break;
		}

		if (*(p = utf8trim(line)))
			add_history((char *)p);

		lexstr(line, &toks);

		da_foreach (&toks, tok)
			printf("token: ‘%.*s’\n", (int)tok->len, tok->p);

		free(line);
		free(toks.buf);
	}
}
