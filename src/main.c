#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <readline/readline.h>

#include "lexer.h"

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
		char *line;

		if (!(line = readline("=> "))) {
			fputs("^D\n", stderr);
			break;
		}
		lexstr(line, NULL);
		free(line);
	}
}
