#include <stddef.h>

#include <bitset.h>
#include <macros.h>
#include <rune.h>

#include "syntax.h"

static constexpr rune spaces[] = {
	0x0009, /* CHARACTER TABULATION */
	0x0020, /* SPACE */
	0x200E, /* LEFT-TO-RIGHT MARK */
	0x200F, /* RIGHT-TO-LEFT MARK */
};

static constexpr rune newlines[] = {
	0x000A, /* LINE FEED */
	0x000B, /* LINE TABULATION */
	0x000C, /* FORM FEED */
	0x000D, /* CARRIAGE RETURN */
	0x0085, /* NEXT LINE */
	0x2028, /* LINE SEPARATOR */
	0x2029, /* PARAGRAPH SEPARATOR */
};

static constexpr char escapes[] = {
	['a'] = '\a', ['b'] = '\b', ['t'] = '\t',   ['n'] = '\n',  ['v'] = '\v',
	['f'] = '\f', ['r'] = '\r', ['e'] = '\x1B', ['\\'] = '\\',
};

static constexpr char escapes_meta[] = {
	['a'] = '\a', ['b'] = '\b', ['t'] = '\t',   ['n'] = '\n',  ['v'] = '\v',
	['f'] = '\f', ['r'] = '\r', ['e'] = '\x1B', ['\\'] = '\\', ['#'] = '#',
	['&'] = '&',  ['|'] = '|',  [';'] = ';',    ['{'] = '{',   ['}'] = '}',
	['('] = '(',  [')'] = ')',
};

/* See gen/meta */
static constexpr bitset(meta, ASCII_MAX + 1) = {
	0x00, 0x00, 0x00, 0x00, 0x48, 0x03, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38,
};

bool
rishws(rune ch)
{
	for (size_t i = 0; i < lengthof(spaces); i++) {
		if (spaces[i] == ch)
			return true;
	}
	return false;
}

bool
risvws(rune ch)
{
	for (size_t i = 0; i < lengthof(newlines); i++) {
		if (newlines[i] == ch)
			return true;
	}
	return false;
}

bool
rismeta(rune ch)
{
	return ch <= ASCII_MAX && TESTBIT(meta, ch);
}

rune
escape(rune ch, bool meta)
{
	if (meta)
		return ch >= lengthof(escapes_meta) ? 0 : escapes_meta[ch];
	return ch >= lengthof(escapes) ? 0 : escapes[ch];
}
