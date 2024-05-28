#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <setjmp.h>
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

#include "error.h"
#include "exec.h"
#include "lexer.h"
#include "parser.h"
#include "syntax.h"

static bool interactive;

static void rloop(void);
static int readfile(FILE *);
static struct u8view ucstrim(struct u8view);

int
main(int, char **argv)
{
	mlib_setprogname(argv[0]);
	errinit();

	setlocale(LC_ALL, "");

	if ((interactive = isatty(STDIN_FILENO)))
		rloop();
	else
		return readfile(stdin);

	return EXIT_SUCCESS;
}

void
rloop(void)
{
	int err, ret = 0;
	static char prompt[256];
	static char histfile[PATH_MAX];

	snprintf(histfile, sizeof(histfile), "%s/.local/share/.andy-hist",
	         getenv("HOME"));
	int fd = open(histfile, O_CREAT | O_RDONLY, 0666);
	if (fd == -1 && errno != EEXIST)
		warn("open: %s:", histfile);
	close(fd);

	if ((err = read_history(histfile)) != 0)
		warn("read_history: %s: %s", histfile, strerror(err));

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

		jmp_buf onerr;
		arena a = mkarena(0);
		struct lexer lexer = {
			.file = "<stdin>",
			.sv = line,
			.base = line.p,
			.err = &onerr,
		};
		if (setjmp(onerr) == 0) {
			struct program *p = parse_program((struct parser){
				.l = &lexer,
				.a = &a,
				.err = &onerr,
			});
			if (p == nullptr)
				warn("failed to parse");
			struct ctx ctx = {
				.fds = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO},
				.a = &a,
			};
			ret = exec_prog(*p, ctx);
		}
		arena_free(&a);

empty:
		free(save);
	}

	if ((err = write_history(histfile)) != 0)
		warn("write_history: %s: %s", histfile, strerror(err));
	rl_clear_history();
}

int
readfile(FILE *stream)
{
	size_t nr;
	dynarr(char) bob = {.alloc = alloc_heap};
	static char buf[BUFSIZ];

	while ((nr = fread(buf, 1, sizeof(buf), stream)) > 0)
		DAEXTEND(&bob, buf, nr);
	if (ferror(stream))
		err("ferror: <stdin>:");

	jmp_buf onerr;
	arena a = mkarena(0);
	struct lexer lexer = {
		.file = "<stdin>",
		.sv = (struct u8view){bob.buf, bob.len},
		.base = bob.buf,
		.err = &onerr,
	};

	if (setjmp(onerr) != 0)
		exit(EXIT_FAILURE);

	struct program *p = parse_program((struct parser){
		.l = &lexer,
		.a = &a,
		.err = &onerr,
	});
	if (p == nullptr)
		warn("failed to parse");
	struct ctx ctx = {
		.fds = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO},
		.a = &a,
	};
	int ret = exec_prog(*p, ctx);

	arena_free(&a);
	free(bob.buf);
	return ret;
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
