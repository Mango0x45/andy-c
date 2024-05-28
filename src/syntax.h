#ifndef ANDY_SYNTAX_H
#define ANDY_SYNTAX_H

#include <rune.h>

/* Test if a rune is horizontal whitespace */
[[unsequenced, nodiscard]] bool rishws(rune);

/* Test if a rune is vertical whitespace */
[[unsequenced, nodiscard]] bool risvws(rune);

/* Test if a rune is a metacharacter */
[[unsequenced, nodiscard]] bool rismeta(rune);

/* Return the rune that the given rune maps to when backslash escaped, or 0.  If
   META is true then also allow escaping metacharacters. */
[[unsequenced, nodiscard]] rune escape(rune, bool meta);

#endif /* !ANDY_SYNTAX_H */
