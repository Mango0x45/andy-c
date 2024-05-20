#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <alloc.h>
#include <dynarr.h>
#include <errors.h>
#include <mbstring.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <unicode/prop.h>

#include "lexer.h"
#include "repr.h"

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
	arena a;
	struct arena_ctx ctx = {.a = &a};

	for (;;) {
		char *save = readline("Andy > ");

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

		a = mkarena(0);
		lextoks toks = {.alloc = alloc_arena, .ctx = &ctx};
		(void)lexstr("<stdin>", line, &toks);

		da_foreach (toks, tok)
			repr(*tok);

		arena_free(&a);
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
