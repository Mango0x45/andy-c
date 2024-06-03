#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <alloc.h>
#include <cli.h>
#include <macros.h>

#include "builtin.h"
#include "exec.h"
#include "symtab.h"
#include "vartab.h"

static int
xwarn(const char *fmt, ...)
{
	int save = errno;
	va_list ap;
	va_start(ap, fmt);
	flockfile(stderr);

	vfprintf(stderr, fmt, ap);
	if (fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(save));
	fputc('\n', stderr);

	funlockfile(stderr);
	va_end(ap);
	return EXIT_FAILURE;
}

int
builtin_cd(char **argv, size_t n)
{
	if (n == 1) {
		char *h = getenv("HOME");
		if (h == nullptr)
			return xwarn("cd: $HOME environment variable not set");
		return chdir(h) == -1 ? xwarn("cd: %s:", h) : EXIT_SUCCESS;
	}

	return chdir(argv[1]) == -1 ? xwarn("cd: %s:", argv[1]) : EXIT_SUCCESS;
}

int
builtin_echo(char **argv, size_t n)
{
	for (size_t i = 1; i < n; i++) {
		if (fputs(argv[i], stdout) == EOF)
			return xwarn("echo:");
		if (i < n - 1) {
			if (putchar(' ') == EOF)
				return xwarn("echo:");
		}
	}
	if (putchar('\n') == EOF)
		return xwarn("echo:");

	return EXIT_SUCCESS;
}

int
builtin_false(char **, size_t)
{
	return EXIT_FAILURE;
}

int
builtin_get(char **argv, size_t n)
{
	if (n < 2)
		return xwarn("Usage: get symbol [key ...]");

	struct u8view sym = {argv[1], strlen(argv[1])};
	struct vartab *vt = symtabget(symboltable, sym);

	if (vt == nullptr)
		return EXIT_SUCCESS;
	for (size_t i = 2; i < n; i++) {
		struct u8view k = {argv[i], strlen(argv[i])};
		struct u8view v = vartabget(*vt, k);
		if (v.p != nullptr)
			printf("%.*s\n", SV_PRI_ARGS(v));
	}

	return EXIT_SUCCESS;
}

int
builtin_set(char **argv, size_t argc)
{
	if (argc < 2) {
usage:
		return xwarn("Usage: set symbol [value ...]\n"
		             "       set -k key symbol [value]");
	}

	struct u8view kflag = {};
	struct optparser cli = mkoptparser(argv);
	static const struct cli_option opts[] = {
		{'k', U8C("key"), CLI_REQ},
	};

	for (rune ch; (ch = optparse(&cli, opts, lengthof(opts))) != 0;) {
		switch (ch) {
		case 'k':
			kflag = cli.optarg;
			break;
		default:
			xwarn("set: %s", cli.errmsg);
			goto usage;
		}
	}

	argc -= cli.optind;
	argv += cli.optind;

	if (kflag.p != nullptr && argc > 2)
		goto usage;

	struct u8view sym = {argv[0], strlen(argv[0])};
	struct vartab *vt = symtabget(symboltable, sym);

	if (vt == nullptr) {
		*(vt = bufalloc(nullptr, 1, sizeof(*vt))) = mkvartab();
		sym.p = memcpy(bufalloc(nullptr, 1, sym.len), argv[0], sym.len);
		symtabadd(&symboltable, sym, vt);
	}

	if (kflag.p != nullptr && argc == 1) {
		vartabdel(vt, kflag);
	} else if (kflag.p != nullptr) {
		struct u8view k = {.len = kflag.len};
		k.p = memcpy(bufalloc(nullptr, 1, k.len), kflag.p, k.len);
		struct u8view v = {.len = strlen(argv[1])};
		v.p = memcpy(bufalloc(nullptr, 1, v.len), argv[1], v.len);
		vartabadd(vt, k, v);
	} else if (argc == 1) {
		/* TODO: Only delete numeric keys */
		/* TODO: Memory leak here? */
		symtabdel(&symboltable, sym);
	} else {
		for (size_t i = 1; i < argc; i++) {
			constexpr size_t SIZE_STR_MAX = 21;

			char8_t *kbuf = bufalloc(nullptr, SIZE_STR_MAX, sizeof(char8_t));
			size_t klen = snprintf(kbuf, SIZE_STR_MAX, "%zu", i - 1);
			kbuf = bufalloc(kbuf, klen, sizeof(char8_t));

			size_t vlen = strlen(argv[i]);
			char8_t *vbuf = bufalloc(nullptr, vlen, sizeof(char8_t));
			memcpy(vbuf, argv[i], vlen);

			struct u8view k = {kbuf, klen};
			struct u8view v = {vbuf, vlen};
			vartabadd(vt, k, v);
		}
	}

	return EXIT_SUCCESS;
}

int
builtin_true(char **, size_t)
{
	return EXIT_SUCCESS;
}
