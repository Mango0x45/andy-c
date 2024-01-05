#ifndef ANDY_UNREACHABLE_H
#define ANDY_UNREACHABLE_H

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

#endif /* !ANDY_UNREACHABLE_H */
