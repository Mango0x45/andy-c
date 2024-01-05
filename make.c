#if __has_include(<features.h>)
#	include <features.h>
#endif

#if defined(__GLIBC__) || defined(__FreeBSD__) || defined(__NetBSD__) \
	|| defined(__DragonFly__)
#	define HAS_STRCHRNUL 1
#endif

#include <errno.h>
#include <glob.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cbs.h"

#define CC "cc"
#define CFLAGS \
	"-Wall", "-Wextra", "-Wpedantic", "-Wno-gnu-binary-literal", "-std=c2x", \
		"-pipe"
#define CFLAGS_DEBUG "-g", "-ggdb3", "-DDEBUG=1"
#define CFLAGS_RELEASE \
	"-Werror", "-O3", "-march=native", "-mtune=native", "-flto"
#define LDFLAGS_RELEASE "-flto"

#define TARGET "andy"

#define streq(x, y) (!strcmp(x, y))
#define cmdprc(c) \
	do { \
		int ec; \
		cmdput(c); \
		if ((ec = cmdexec(c))) \
			diex("%s terminated with exit-code %d", *c._argv, ec); \
		cmdclr(&c); \
	} while (0)

static int globerr(const char *, int);
static char *ctoo(const char *);
static void usage(void);
static void build(void);

static bool dflag;
static char *argv0;

void
usage(void)
{
	fprintf(stderr,
	        "Usage: %1$s [-d]\n"
	        "       %1$s clean\n"
	        "       %1$s includes\n",
	        argv0);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	int opt;
	char *p;

	cbsinit(argc, argv);
	rebuild();

	if (!(argv0 = strdup(*argv)))
		die("strdup");
	argv0 = basename(argv0);

	while ((opt = getopt(argc, argv, "d")) != -1) {
		switch (opt) {
		case 'd':
			dflag = true;
			break;
		default:
			usage();
		}
	}

	if ((p = getenv("ANDY_DEV")) && *p)
		dflag = true;

	argc -= optind;
	argv += optind;

	if (chdir(dirname(*(argv - optind))) == -1)
		die("chdir: %s", *(argv - optind));

	if (argc > 0) {
		if (streq("clean", *argv)) {
			cmd_t c = {0};
			cmdadd(&c, "find", ".", "(", "-name", TARGET, "-or", "-name", "*.o",
			       ")", "-delete");
			cmdprc(c);
		} else if (streq("includes", *argv)) {
			cmd_t c = {};
			cmdadd(&c, "find", "src", "-name", "*.c", "-exec",
			       "include-what-you-use", "-std=c2x", "{}", ";");
			cmdprc(c);
		} else {
			fprintf(stderr, "%s: invalid subcommand -- '%s'\n", argv0, *argv);
			usage();
		}
	} else
		build();

	return EXIT_SUCCESS;
}

int
globerr(const char *s, int e)
{
	errno = e;
	die("glob: %s", s);
}

char *
ctoo(const char *s)
{
	char *o = strdup(s);
	size_t n = strlen(s);
	if (!o)
		die("strdup");
	o[n - 1] = 'o';
	return o;
}

void
build(void)
{
	int rv;
	glob_t g;
	cmd_t c = {0};
	struct strv v = {0};

	pcquery(&v, "readline", PKGC_CFLAGS);

	if ((rv = glob("src/*.c", 0, globerr, &g)) && rv != GLOB_NOMATCH)
		die("glob");

	for (size_t i = 0; i < g.gl_pathc; i++) {
		char *src, *dst;
		src = g.gl_pathv[i];
		dst = ctoo(src);

		if (foutdated(dst, src)) {
			cmdadd(&c, CC, CFLAGS);
			if (dflag)
				cmdadd(&c, CFLAGS_DEBUG);
			else
				cmdadd(&c, CFLAGS_RELEASE);

			/* <uchar.h> is part of C11 but Apple doesn’t provide it. */
#if !__has_include(<uchar.h>)
			cmdadd(&c, "-Isrc/compat");
#endif

			if (streq("src/main.c", src))
				cmdaddv(&c, v.buf, v.len);
#if HAS_STRCHRNUL
			else if (streq("src/utf8.c", src)) {
				cmdadd(&c, "-DHAS_STRCHRNUL=1");
#	if __GLIBC__
				cmdadd(&c, "-D_GNU_SOURCE");
#	endif
			}
#endif
			cmdadd(&c, "-o", dst, "-c", src);
			cmdprc(c);
		}

		free(dst);
	}

	for (size_t i = 0; i < g.gl_pathc; i++)
		g.gl_pathv[i][strlen(g.gl_pathv[i]) - 1] = 'o';

	if (foutdatedv(TARGET, (const char **)g.gl_pathv, g.gl_pathc)) {
		strvfree(&v);
		cmdadd(&c, CC);
		if (pcquery(&v, "readline", PKGC_LIBS))
			cmdaddv(&c, v.buf, v.len);
		else
			cmdadd(&c, "-lreadline");
		if (!dflag)
			cmdadd(&c, LDFLAGS_RELEASE);
		cmdadd(&c, "-o", TARGET);
		cmdaddv(&c, g.gl_pathv, g.gl_pathc);
		cmdprc(c);
	}

	free(c._argv);
	globfree(&g);
	strvfree(&v);
}
