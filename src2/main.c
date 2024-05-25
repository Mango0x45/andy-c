#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errors.h>
#include <mbstring.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <unicode/prop.h>

#include "exec.h"
#include "lexer.h"
#include "parser.h"
#include "syntax.h"

static bool interactive;

static void rloop(void);
static struct u8view ucstrim(struct u8view);

int
main(int, char **argv)
{
	mlib_setprogname(argv[0]);

	setlocale(LC_ALL, "");

	if ((interactive = isatty(STDOUT_FILENO)))
		rloop();

	return EXIT_SUCCESS;
}

void
rloop(void)
{
	int ret = 0;
	static char prompt[256];

	for (;;) {
		snprintf(prompt, sizeof(prompt), "[%d] > ", ret);
		char *save = readline(prompt);

		if (save == nullptr) {
			fputs("^D\n", stderr);
			break;
		}

		struct u8view line = {save, strlen(save)};
		line = ucstrim(line);
		if (line.len == 0)
			goto empty;

		((char8_t *)line.p)[line.len] = '\0';
		add_history(line.p);

		struct lexer lexer = {
			.file = "<stdin>",
			.sv = line,
			.base = line.p,
		};

#if 1
		arena a = mkarena(0);
		struct program *p = parse_program(lexer, &a);
		if (p == nullptr)
			warn("failed to parse");
		struct ctx ctx = {
			.fds = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO},
			.a = &a,
		};
		ret = exec_prog(*p, ctx);
		arena_free(&a);
#else
		struct lextok tok;
		do {
			tok = lexnext(&lexer);
			repr(tok);
		} while (tok.kind != LTK_EOF && tok.kind != LTK_ERR);
#endif

empty:
		free(save);
	}
}

struct u8view
ucstrim(struct u8view sv)
{
	int w;
	for (rune ch; (w = ucsnext(&ch, &sv)) != 0;) {
		if (!rishws(ch)) {
			VSHFT(&sv, -w);
			break;
		}
	}

	const char8_t *p = sv.p + sv.len;
	for (rune ch; (w = ucsprev(&ch, (const char8_t **)&p, sv.p)) != 0;) {
		if (!rishws(ch)) {
			sv.len = p - sv.p + w;
			break;
		}
	}

	return sv;
}
