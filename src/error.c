#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errors.h>
#include <macros.h>
#include <mbstring.h>
#include <unicode/prop.h>
#include <unicode/string.h>

#include "error.h"
#include "syntax.h"

#define TABSIZE 8
#define SPACES  "        "

[[unsequenced, nodiscard]] static int sizelen(size_t);

/* clang-format off */
static bool color;
static const char *_bold = "\33[1m",
                  *_done = "\33[0m",
                  *_err  = "\33[1;31m",
                  *_warn = "\33[1;35m";
/* clang-format on */

void
errinit(void)
{
	if (isatty(STDERR_FILENO)) {
		const char *ev = getenv("NO_COLOR");
		color = ev == nullptr || ev[0] == 0;
	}

	if (!color)
		_bold = _done = _err = _warn = "";
}

void
diagemit(const char *type, const char *file, const char8_t *base,
         struct u8view hl, size_t off, const char *fmt, ...)
{
	const char *_type = streq(type, "warning") ? _warn : _err;

	flockfile(stderr);

	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s%s: %s:%zu:%s %s%s:%s ", _bold, mlib_progname(), file,
	        off, _done, _type, type, _done);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);

	int w;
	size_t ln = 1;
	const char8_t *start = base, *end;

	{
		/* We assume here that the input is null-terminated (which it is) */

		rune ch;
		const char8_t *p = start;

		for (;;) {
			int w = ucstor(&ch, p);
			if (ch == 0 || (p >= hl.p + hl.len && risvws(ch))) {
				end = p;
				break;
			}
			if (p < hl.p && risvws(ch)) {
				ln++;
				start = p;
			}
			p += w;
		}
	}

	w = sizelen(ln);
	w = MAX(w, 4);

	fprintf(stderr, " %*zu │ ", w, ln);

	rune ch;
	struct u8view line = {start, end - start};
	for (int w, col = 0; (w = ucsnext(&ch, &line)) != 0;) {
		if (line.p - w == hl.p)
			fputs(_type, stderr);
		if (line.p - w == hl.p + hl.len)
			fputs(_done, stderr);

		if (ch == '\t') {
			col += w = TABSIZE - (col & (TABSIZE - 1));
			fprintf(stderr, "%.*s", w, SPACES);
		} else {
			fprintf(stderr, "%.*s", w, line.p - w);
			w = uprop_get_wdth(ch);
			col += w = MAX(w, 0);
		}
	}
	if (hl.p + hl.len == end)
		fputs(_done, stderr);

	fputc('\n', stderr);
	fprintf(stderr, " %*c │ ", w, ' ');

	line.p = start;
	line.len = hl.p - start;
	size_t pre_hl_cols = ucswdth(line, TABSIZE);
	line.len += hl.len;
	size_t pre_and_hl_cols = ucswdth(line, TABSIZE);

	for (size_t i = 0; i < pre_hl_cols; i++)
		fputc(' ', stderr);

	fprintf(stderr, "%s^", _type);
	for (size_t i = 1; i < pre_and_hl_cols - pre_hl_cols; i++)
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
