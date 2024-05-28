#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <errors.h>
#include <macros.h>
#include <mbstring.h>
#include <unicode/string.h>

#include "error.h"
#include "syntax.h"

#define TAB_AS_SPACES "        "

[[unsequenced, nodiscard]] static int sizelen(size_t);

/* clang-format off */
static bool color;
static const char *_bold = "\33[1m",
                  *_done = "\33[0m",
                  *_err  = "\33[1;31m";
/* clang-format on */

void
errinit(void)
{
	if (isatty(STDERR_FILENO)) {
		const char *ev = getenv("NO_COLOR");
		color = ev == nullptr || ev[0] == 0;
	}

	if (!color)
		_bold = _done = _err = "";
}

void
erremit(const char *file, const char8_t *base, struct u8view hl, size_t off,
        const char *fmt, ...)
{
	flockfile(stderr);

	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s%s: %s:%zu:%s %serror:%s ", _bold, mlib_progname(), file,
	        off, _done, _err, _done);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);

	int w;
	size_t ln = 1;
	const char8_t *start = base, *end;

	{
		rune ch;
		struct u8view tmp = {base, off + 1};
		while (ucsnext(&ch, &tmp) != 0) {
			if (risvws(ch)) {
				ln++;
				start = tmp.p;
			}
		}

		for (;;) {
			int w = ucstor(&ch, tmp.p);
			if (ch == 0 || risvws(ch)) {
				end = tmp.p;
				break;
			}
			tmp.p += w;
		}
	}

	w = sizelen(ln);
	w = MAX(w, 4);

	fprintf(stderr, " %*zu │ ", w, ln);

	for (const char8_t *p = start; p < end; p++) {
		if (p == hl.p)
			fputs(_err, stderr);

		if (*p == '\t')
			fputs(TAB_AS_SPACES, stderr);
		else
			fputc(*p, stderr);

		if (p == hl.p + hl.len || p == end - 1)
			fputs(_done, stderr);
	}

	fputc('\n', stderr);
	fprintf(stderr, " %*c │ ", w, ' ');

	for (const char8_t *p = start; p < hl.p; p++) {
		if (*p == '\t')
			fputs(TAB_AS_SPACES, stderr);
		else
			fputc(' ', stderr);
	}

	fprintf(stderr, "%s^", _err);
	for (size_t i = 1, cols = ucsgcnt(hl); i < cols; i++)
		fputc('~', stderr);
	fprintf(stderr, "%s\n", _done);
	funlockfile(stderr);
}

int
sizelen(size_t x)
{
	int n;
	for (n = 0; x; x /= 10, n++)
		;
	return n;
}
