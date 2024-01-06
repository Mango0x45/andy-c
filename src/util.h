#ifndef ANDY_UTIL_H
#define ANDY_UTIL_H

/* In release builds we want to make use of the optimization benefits of
   unreachable(), but in debug builds we should print error messages on failure
   to help catch potential bugs. */
#if DEBUG
#	include <err.h>
#	ifdef unreachable
#		undef unreachable
#	endif
#	define unreachable() \
		errx(1, "%s:%d: Reached unreachable() in %s()", __FILE__, __LINE__, \
		     __func__)
#elifndef unreachable
#	include <stddef.h>
#endif

/* Classic min and max macros */
#define min(α, β) ((α) < (β) ? (α) : (β))
#define max(α, β) ((α) > (β) ? (α) : (β))

/* Length of static arrays */
#define lengthof(α) (sizeof(α) / sizeof(*(α)))

/* Mark a branch as likely */
#ifdef __GNUC__
#	define LIKELY(α) __builtin_expect((α), 1)
#else
#	define LIKELY(α)
#endif

#endif /* !ANDY_UTIL_H */
