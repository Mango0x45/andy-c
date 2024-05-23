#include <sys/wait.h>

#include <string.h>
#include <unistd.h>

#include <errors.h>
#include <macros.h>

#include "exec.h"

static void exec_expr(struct expr, arena *);

void
exec_prog(struct program p, arena *a)
{
	da_foreach (p, e)
		exec_expr(*e, a);
}

void
exec_expr(struct expr e, arena *a)
{
	ASSUME(e.kind == EK_BASIC);
	ASSUME(e.kind != EK_INVAL);

	char **argv = arena_new(a, char *, e.b.len + 1);
	if (argv == nullptr)
		err("arena_new:");
	argv[e.b.len] = nullptr;

	for (size_t i = 0; i < e.b.len; i++) {
		ASSUME(e.b.buf[i].kind == VK_ARG);
		struct u8view sv = e.b.buf[i].arg;

		argv[i] = arena_new(a, char, sv.len + 1);
		if (argv[i] == nullptr)
			err("arena_new:");
		memcpy(argv[i], sv.p, sv.len);	
		argv[i][sv.len] = '\0';
	}

	pid_t pid = fork();
	switch (pid) {
	case -1:
		err("fork:");
	case 0:
		execvp(argv[0], argv);
		err("%s: execvp:", argv[0]);
	default:
		if (waitpid(pid, nullptr, 0) == -1)
			err("%d: waitpid:", (int)pid);
	}
}
