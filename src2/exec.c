#include <sys/wait.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errors.h>
#include <macros.h>

#include "builtin.h"
#include "exec.h"

static int exec_expr(struct expr, struct ctx ctx);
static int exec_pipe(struct expr, struct ctx ctx);
static int exec_basic(struct expr, struct ctx ctx);

int
exec_prog(struct program p, struct ctx ctx)
{
	da_foreach (p, e) {
		int ret = exec_expr(*e, ctx);
		if (ret != EXIT_SUCCESS)
			return ret;
	}
	return EXIT_SUCCESS;
}

int
exec_expr(struct expr e, struct ctx ctx)
{
	ASSUME(e.kind != EK_INVAL);
	switch (e.kind) {
	case EK_BASIC:
		return exec_basic(e, ctx);
	case EK_BINOP:
		switch (e.bo.op) {
		case '|':
			return exec_pipe(e, ctx);
		}
	case EK_INVAL:
	}

	return -1;
}

int
exec_pipe(struct expr e, struct ctx ctx)
{
	int fds[2];
	enum {
		R,
		W,
	};

	if (pipe(fds) == -1)
		err("pipe:");

	struct ctx lctx, rctx;
	lctx = rctx = ctx;

	lctx.fds[STDOUT_FILENO] = fds[W];
	rctx.fds[STDIN_FILENO]  = fds[R];

	exec_expr(*e.bo.lhs, lctx);
	close(fds[W]);

	int ret = exec_expr(*e.bo.rhs, rctx);
	close(fds[R]);
	return ret;
}

int
exec_basic(struct expr e, struct ctx ctx)
{
	struct basic b = e.b;

	char **argv = arena_new(ctx.a, char *, b.len + 1);
	if (argv == nullptr)
		err("arena_new:");
	argv[b.len] = nullptr;

	for (size_t i = 0; i < e.b.len; i++) {
		ASSUME(b.buf[i].kind == VK_ARG);
		struct u8view sv = b.buf[i].arg;

		argv[i] = arena_new(ctx.a, char, sv.len + 1);
		if (argv[i] == nullptr)
			err("arena_new:");
		memcpy(argv[i], sv.p, sv.len);
		argv[i][sv.len] = '\0';
	}

	builtin_fn fn = lookup_builtin(argv[0]);
	if (fn != nullptr)
		return fn(argv, b.len);

	pid_t pid = fork();
	if (pid == -1)
		err("fork:");

	/* Parent process */
	if (pid != 0) {
		int ws;
		if (waitpid(pid, &ws, 0) == -1)
			err("%d: waitpid:", (int)pid);
		return WIFEXITED(ws) ? WEXITSTATUS(ws) : UINT8_MAX + 1;
	}

	for (int i = 0; i < (int)lengthof(ctx.fds); i++) {
		if (ctx.fds[i] != i) {
			if (dup2(ctx.fds[i], i) == -1)
				err("dup2:");
		}
	}

	execvp(argv[0], argv);
	err("%s: execvp:", argv[0]);
}
