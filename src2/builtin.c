#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errors.h>

#include "builtin.h"

static int
xwarn(const char *argv0, const char *fmt, ...)
{
	const char *save = mlib_progname();
	mlib_setprogname(argv0);

	va_list ap;
	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);

	mlib_setprogname(save);
	return EXIT_FAILURE;
}

int
builtin_cd(char **argv, size_t n)
{
	if (n == 1) {
		char *h = getenv("HOME");
		if (h == nullptr)
			return xwarn(argv[0], "$HOME environment variable not set");
		return chdir(h) == -1 ? xwarn(argv[0], "%s:", h) : EXIT_SUCCESS;
	}

	return chdir(argv[1]) == -1 ? xwarn(argv[0], "%s:", argv[1]) : EXIT_SUCCESS;
}

int
builtin_echo(char **argv, size_t n)
{
	for (size_t i = 1; i < n; i++) {
		if (fputs(argv[i], stdout) == EOF)
			return xwarn(argv[0], ":");
		if (i < n - 1) {
			if (putchar(' ') == EOF)
				return xwarn(argv[0], ":");
		}
	}
	if (putchar('\n') == EOF)
			return xwarn(argv[0], ":");

	return EXIT_SUCCESS;
}

int
builtin_false(char **, size_t)
{
	return EXIT_FAILURE;
}

int
builtin_true(char **, size_t)
{
	return EXIT_SUCCESS;
}
