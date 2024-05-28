#define _GNU_SOURCE
#include <errno.h>
#if __has_include(<features.h>)
#	include <features.h>
#endif
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CBS_PTHREAD
#include "cbs.h"

#define CC     "cc"
#define TARGET "andy"

#define CFLAGS_ALL                                                             \
	WARNINGS, "-pipe", "-std=c23", "-Ivendor/mlib/include" GLIB_EXTRAS
#define CFLAGS_DBG "-g", "-ggdb3", "-O0", "-fsanitize=address,undefined"
#define CFLAGS_RLS "-O3", "-flto", "-DNDEBUG" NOT_APPLE_EXTRAS

#define WARNINGS                                                               \
	"-Wall", "-Wextra", "-Wpedantic", "-Wno-attributes", "-Wvla",              \
		"-Wno-pointer-sign", "-Wno-parentheses"

#ifdef __GLIBC__
#	define GLIB_EXTRAS , "-D_GNU_SOURCE"
#else
#	define GLIB_EXTRAS
#endif

#ifndef __APPLE__
#	define NOT_APPLE_EXTRAS , "-march=native", "-mtune=native"
#else
#	define NOT_APPLE_EXTRAS
#endif

#define CMDRC(c)                                                               \
	do {                                                                       \
		int ec;                                                                \
		if ((ec = cmdexec(c)) != EXIT_SUCCESS)                                 \
			diex("%s terminated with exit-code %d", *(c)._argv, ec);           \
		cmdclr(&(c));                                                          \
	} while (false)

#define flagset(o)  (flags & (1 << ((o) - 'a')))
#define streq(x, y) (!strcmp(x, y))

static int globerr(const char *, int);
static void usage(void);
static void work(void *);

static bool dflag;
static char *argv0;

static unsigned long flags;

void
usage(void)
{
	fprintf(stderr,
	        "Usage: %1$s [-afpr]\n"
	        "       %1$s clean\n",
	        argv0);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	cbsinit(argc, argv);
	rebuild();

	argv0 = argv[0];

	int opt;
	while ((opt = getopt(argc, argv, "afpr")) != -1) {
		switch (opt) {
		case '?':
			usage();
		case 'a':
			/* -a implies -f */
			flags |= 1 << ('f' - 'a');
			[[fallthrough]];
		default:
			flags |= 1 << (opt - 'a');
		}
	}

	argc -= optind;
	argv += optind;

	if (argc >= 1) {
		cmd_t c = {};
		if (streq("clean", *argv)) {
			cmdadd(&c, "find", ".", "(", "-name", TARGET, "-or", "-name",
			       "*.[ao]", ")", "-delete");
			cmdput(c);
			CMDRC(c);
		} else {
			fprintf(stderr, "%s: invalid subcommand — ‘%s’\n", argv0, *argv);
			usage();
		}
	} else {
		int err;
		cmd_t c = {};
		glob_t g;
		tpool_t tp;

		if (!fexists("./vendor/mlib/make")) {
			struct strv sv = {};
			env_or_default(&sv, "CC", CC);
			cmdaddv(&c, sv.buf, sv.len);
			cmdadd(&c, "-lpthread", "-o", "./vendor/mlib/make",
			       "./vendor/mlib/make.c");
			CMDRC(c);
		}

		cmdadd(&c, "./vendor/mlib/make");
		if (flagset('a'))
			cmdadd(&c, "-f");
		if (flagset('p'))
			cmdadd(&c, "-p");
		if (flagset('r'))
			cmdadd(&c, "-r");
		CMDRC(c);

		if ((err = glob("src/*.c", 0, globerr, &g)) != 0 && err != GLOB_NOMATCH)
		{
			die("glob");
		}

		int procs = nproc();
		if (procs == -1) {
			if (errno)
				die("nproc");
			procs = 8;
		}

		/* Build the builtins hash table */
		if (flagset('f') || foutdated("src/builtin.gen.c", "src/builtin.gperf"))
		{
			cmdadd(&c, "gperf", "src/builtin.gperf",
			       "--output-file=src/builtin.gen.c");
			if (flagset('p'))
				cmdput(c);
			else
				fprintf(stderr, "GPERF\t%s\n", "src/builtin.gen.c");
			CMDRC(c);
		}

		tpinit(&tp, procs);
		for (size_t i = 0; i < g.gl_pathc; i++)
			tpenq(&tp, work, g.gl_pathv[i], nullptr);
		tpwait(&tp);
		tpfree(&tp);

		for (size_t i = 0; i < g.gl_pathc; i++)
			g.gl_pathv[i][strlen(g.gl_pathv[i]) - 1] = 'o';

		if (flagset('f')
		    || foutdatedv(TARGET, (const char **)g.gl_pathv, g.gl_pathc))
		{
			struct strv sv = {}, pc = {};
			env_or_default(&sv, "CC", CC);
			if (flagset('r'))
				env_or_default(&sv, "CFLAGS", CFLAGS_RLS);
			else
				env_or_default(&sv, "CFLAGS", CFLAGS_DBG);

			cmdaddv(&c, sv.buf, sv.len);
			if (pcquery(&pc, "readline", PKGC_CFLAGS | PKGC_LIBS))
				cmdaddv(&c, pc.buf, pc.len);
			else
				cmdadd(&c, "-lreadline");

			cmdaddv(&c, g.gl_pathv, g.gl_pathc);
			cmdadd(&c, "./vendor/mlib/libmlib.a", "-o", TARGET);
			if (flagset('p'))
				cmdput(c);
			else
				fprintf(stderr, "CC\t%s\n", TARGET);
			CMDRC(c);

			strvfree(&sv);
			strvfree(&pc);
		}

		globfree(&g);
	}

	return EXIT_SUCCESS;
}

int
globerr(const char *s, int e)
{
	errno = e;
	die("glob: %s", s);
}

void
work(void *p)
{
	char *dst, *src = p;
	cmd_t c = {};
	struct strv sv = {};

	if (!(dst = strdup(src)))
		die("strdup");
	dst[strlen(dst) - 1] = 'o';

	if (flagset('f') || foutdated(dst, src)) {
		env_or_default(&sv, "CC", CC);
		if (flagset('r'))
			env_or_default(&sv, "CFLAGS", CFLAGS_RLS);
		else
			env_or_default(&sv, "CFLAGS", CFLAGS_DBG);
		cmdaddv(&c, sv.buf, sv.len);
		cmdadd(&c, CFLAGS_ALL, "-o", dst, "-c", src);
		if (flagset('p'))
			cmdput(c);
		else
			fprintf(stderr, "CC\t%s\n", dst);
		CMDRC(c);
	}

	strvfree(&sv);
	free(dst);
}
