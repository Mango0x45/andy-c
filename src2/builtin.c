#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"

static int
xwarn(struct ctx ctx, const char *fmt, ...)
{
	int save = errno;
	FILE *fp = fdopen(ctx.fds[STDERR_FILENO], "w");
	if (fp != nullptr) {
		va_list ap;
		va_start(ap, fmt);
		flockfile(fp);
		vfprintf(fp, fmt, ap);
		if (fmt[strlen(fmt) - 1] == ':')
			fprintf(fp, " %s", strerror(save));
		fputc('\n', fp);
		funlockfile(fp);
		va_end(ap);
	}

	return EXIT_FAILURE;
}

int
builtin_cd(char **argv, size_t n, struct ctx ctx)
{
	if (n == 1) {
		char *h = getenv("HOME");
		if (h == nullptr)
			return xwarn(ctx, "cd: $HOME environment variable not set");
		return chdir(h) == -1 ? xwarn(ctx, "cd: %s:", h) : EXIT_SUCCESS;
	}

	return chdir(argv[1]) == -1 ? xwarn(ctx, "cd: %s:", argv[1]) : EXIT_SUCCESS;
}

int
builtin_echo(char **argv, size_t n, struct ctx ctx)
{
	FILE *fp = fdopen(ctx.fds[STDOUT_FILENO], "w");
	if (fp == nullptr)
		return xwarn(ctx, "echo:");

	for (size_t i = 1; i < n; i++) {
		if (fputs(argv[i], fp) == EOF)
			return xwarn(ctx, "echo:");
		if (i < n - 1) {
			if (fputc(' ', fp) == EOF)
				return xwarn(ctx, "echo:");
		}
	}
	if (fputc('\n', fp) == EOF)
		return xwarn(ctx, "echo:");

	return EXIT_SUCCESS;
}

int
builtin_false(char **, size_t, struct ctx)
{
	return EXIT_FAILURE;
}

int
builtin_true(char **, size_t, struct ctx)
{
	return EXIT_SUCCESS;
}
