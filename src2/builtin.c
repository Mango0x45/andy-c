#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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
builtin_echo(char **argv, size_t n)
{
	for (size_t i = 1; i < n - 1; i++) {
		if (printf("%s ", argv[i]) < 0)
			return xwarn(argv[0], "printf:");
	}

	if (puts(argv[n - 1]) == EOF)
		return xwarn(argv[0], "puts:");

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
