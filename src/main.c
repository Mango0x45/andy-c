#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <readline/readline.h>

static bool interactive;

static void rloop(void);

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	interactive = isatty(STDOUT_FILENO);

	if (interactive)
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
		fprintf(stderr, "got the line ‘%s’\n", line);
		free(line);
	}
}
