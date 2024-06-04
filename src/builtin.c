#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <alloc.h>
#include <cli.h>
#include <macros.h>
#include <mbstring.h>
#include <unicode/string.h>

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
builtin_get(char **argv, size_t argc)
{
	bool eflag, kflag, Nflag, vflag;
	struct optparser cli = mkoptparser(argv);
	static const struct cli_option opts[] = {
		{'e', U8C("environment"),  CLI_NONE},
		{'k', U8C("keys"),         CLI_NONE},
		{'N', U8C("no-normalize"), CLI_NONE},
		{'v', U8C("values"),       CLI_NONE},
	};

	eflag = kflag = Nflag = vflag = false;
	for (rune ch; (ch = optparse(&cli, opts, lengthof(opts))) != 0;) {
		switch (ch) {
		case 'e':
			eflag = true;
			break;
		case 'k':
			kflag = true;
			break;
		case 'N':
			Nflag = true;
			break;
		case 'v':
			vflag = true;
			break;
		default:
			xwarn("get: %s", cli.errmsg);
usage:
			return xwarn("Usage: get [-N] symbol [key ...]\n"
			             "       get [-N] -k | -v symbol\n"
			             "       get [-N] -e symbol");
		}
	}

	argc -= cli.optind;
	argv += cli.optind;

	if (kflag && vflag)
		goto usage;
	if ((kflag || vflag) && argc != 1)
		goto usage;
	if ((kflag || vflag) && eflag)
		goto usage;
	if (!kflag && !vflag && argc == 0)
		goto usage;

	struct u8view sym = {argv[0], strlen(argv[0])};
	if (eflag)
		sym.len++;
	if (!Nflag)
		sym.p = ucsnorm(&sym.len, sym, alloc_heap, nullptr, NF_NFC);

	if (eflag) {
		char *ev = getenv(sym.p);
		if (ev != nullptr)
			puts(ev);
		goto out;
	}

	struct vartab *vt = symtabget(symboltable, sym);

	if (vt == nullptr)
		goto out;

	if (kflag) {
		for (size_t i = 0; i < vt->cap; i++) {
			da_foreach (vt->bkts[i], kv)
				printf("%.*s\n", SV_PRI_ARGS(kv->k));
		}
	} else if (vflag) {
		for (size_t i = 0; i < vt->cap; i++) {
			da_foreach (vt->bkts[i], kv)
				printf("%.*s\n", SV_PRI_ARGS(kv->v));
		}
	} else {
		for (size_t i = 1; i < argc; i++) {
			struct u8view k = {argv[i], strlen(argv[i])};
			struct u8view *v = vartabget(*vt, k);
			if (v != nullptr)
				printf("%.*s\n", SV_PRI_ARGS(*v));
		}
	}

out:
	if (!Nflag)
		free((void *)sym.p);
	return EXIT_SUCCESS;
}

int
builtin_set(char **argv, size_t argc)
{
	if (argc < 2) {
usage:
		return xwarn("Usage: set [-N] symbol [value ...]\n"
		             "       set [-N] [-e | -k key] symbol [value]");
	}

	bool eflag, Nflag;
	struct u8view kflag = {};
	struct optparser cli = mkoptparser(argv);
	static const struct cli_option opts[] = {
		{'e', U8C("environment"),  CLI_NONE},
		{'k', U8C("key"),          CLI_REQ },
		{'N', U8C("no-normalize"), CLI_NONE},
	};

	eflag = Nflag = false;
	for (rune ch; (ch = optparse(&cli, opts, lengthof(opts))) != 0;) {
		switch (ch) {
		case 'e':
			eflag = true;
			break;
		case 'k':
			kflag = cli.optarg;
			break;
		case 'N':
			Nflag = true;
			break;
		default:
			xwarn("set: %s", cli.errmsg);
			goto usage;
		}
	}

	argc -= cli.optind;
	argv += cli.optind;

	if ((eflag || kflag.p != nullptr) && argc > 2)
		goto usage;

	bool do_free = !Nflag;
	struct u8view sym = {argv[0], strlen(argv[0])};
	if (eflag)
		sym.len++;
	if (!Nflag)
		sym.p = ucsnorm(&sym.len, sym, alloc_heap, nullptr, NF_NFC);

	if (eflag) {
		if (argc < 2) {
			if (unsetenv(sym.p) == -1)
				warn("unsetenv: %s:", sym.p);
		} else {
			if (setenv(sym.p, argv[1], true) == -1)
				warn("setenv: %s:", sym.p);
		}
	}

	struct vartab *vt = symtabget(symboltable, sym);

	if (vt == nullptr) {
		if (Nflag) {
			sym.p = memcpy(bufalloc(nullptr, sym.len, sizeof(char8_t)), sym.p,
			               sym.len);
		}

		/* Don’t free SYM if we’re adding it to the symbol table, because that
		   would obviously break the symbol table */
		vt = symtabadd(&symboltable, sym, mkvartab());
		do_free = false;
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
		symtabdel(&symboltable, sym);
	} else {
		vartabfree(*vt);
		*vt = mkvartab();

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

	if (do_free)
		free((void *)sym.p);
	return EXIT_SUCCESS;
}

int
builtin_true(char **, size_t)
{
	return EXIT_SUCCESS;
}
