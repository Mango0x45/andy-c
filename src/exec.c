#include <sys/wait.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <alloc.h>
#include <dynarr.h>
#include <errors.h>
#include <macros.h>

#include "exec.h"
#include "parser.h"

struct strs {
	dafields(char *)
};

struct strarr {
	char **p;
	size_t n;
};

static int exec_stmt(struct stmt, struct ctx);
static int exec_andor(struct andor, struct ctx);
static int exec_pipe(struct pipe, struct ctx);
static int exec_unit(struct unit, struct ctx);
static int exec_cmd(struct cmd, struct ctx);
static pid_t exec_cmd_async(struct cmd, struct ctx);
static struct strarr valtostrs(struct value, alloc_fn, void *);

int
exec_prog(struct program p, struct ctx ctx)
{
	da_foreach (p, e) {
		int ret = exec_stmt(*e, ctx);
		if (ret != EXIT_SUCCESS)
			return ret;
	}
	return EXIT_SUCCESS;
}

int
exec_stmt(struct stmt stmt, struct ctx ctx)
{
	switch (stmt.kind) {
	case SK_ANDOR:
		return exec_andor(stmt.ao, ctx);
	}
	unreachable();
}

int
exec_andor(struct andor ao, struct ctx ctx)
{
	ASSUME(ao.r != nullptr);

	int ret;
	switch (ao.op) {
	case '&':
		ret = exec_andor(*ao.l, ctx);
		if (ret == EXIT_SUCCESS)
			ret = exec_pipe(*ao.r, ctx);
		break;
	case '|':
		ret = exec_andor(*ao.l, ctx);
		if (ret != EXIT_SUCCESS)
			ret = exec_pipe(*ao.r, ctx);
		break;
	default:
		return exec_pipe(*ao.r, ctx);
	}

	return ret;
}

int
exec_pipe(struct pipe p, struct ctx ctx)
{
	ASSUME(p.len > 0);

	if (p.len == 1)
		return exec_unit(p.buf[0], ctx);

	size_t cmdcnt = 0;
	da_foreach (p, u) {
		if (u->kind == UK_CMD)
			cmdcnt++;
	}

	pid_t *pids = arena_new(ctx.a, pid_t, cmdcnt);
	if (pids == nullptr)
		err("arena_new:");

	int fds[2];
	enum { R, W };

	for (size_t i = 0; i < p.len; i++) {
		struct ctx nctx = ctx;
		if (i > 0)
			nctx.fds[STDIN_FILENO] = fds[R];
		if (i < p.len - 1) {
			if (pipe(fds) == -1)
				err("pipe:");
			nctx.fds[STDOUT_FILENO] = fds[W];
		}
		pids[i] = exec_cmd_async(p.buf[i].c, nctx);

		if (nctx.fds[STDIN_FILENO] != ctx.fds[STDIN_FILENO])
			close(nctx.fds[STDIN_FILENO]);
		if (nctx.fds[STDOUT_FILENO] != ctx.fds[STDOUT_FILENO])
			close(nctx.fds[STDOUT_FILENO]);
	}

	for (size_t i = 0; i < cmdcnt; i++) {
		int ws;
		if (waitpid(pids[i], &ws, 0) == -1)
			err("waitpid:");
		if (i == cmdcnt - 1)
			return WIFEXITED(ws) ? WEXITSTATUS(ws) : UINT8_MAX + 1;
	}

	unreachable();
}

int
exec_unit(struct unit u, struct ctx ctx)
{
	switch (u.kind) {
	case UK_CMD:
		return exec_cmd(u.c, ctx);
	}
	unreachable();
}

pid_t
exec_cmd_async(struct cmd c, struct ctx ctx)
{
	arena a = mkarena(0);
	struct arena_ctx a_ctx = {.a = &a};
	struct strs argv = {
		.alloc = alloc_arena,
		.ctx = &a_ctx,
	};

	DAGROW(&argv, c.len + 1);
	da_foreach (c, v) {
		struct strarr xs = valtostrs(*v, alloc_arena, &a_ctx);
		DAEXTEND(&argv, xs.p, xs.n);
	}
	DAPUSH(&argv, nullptr);

	pid_t pid = fork();
	if (pid == -1)
		err("fork:");
	if (pid != 0) {
		arena_free(&a);
		return pid;
	}

	for (int i = 0; i < (int)lengthof(ctx.fds); i++) {
		if (ctx.fds[i] != i) {
			close(i);
			if (dup2(ctx.fds[i], i) == -1)
				err("dup2:");
			close(ctx.fds[i]);
		}
	}
	execvp(argv.buf[0], argv.buf);
	err("exec:");
}

int
exec_cmd(struct cmd c, struct ctx ctx)
{
	arena a = mkarena(0);
	struct arena_ctx a_ctx = {.a = &a};
	struct strs argv = {
		.alloc = alloc_arena,
		.ctx = &a_ctx,
	};

	DAGROW(&argv, c.len + 1);
	da_foreach (c, v) {
		struct strarr xs = valtostrs(*v, alloc_arena, &a_ctx);
		DAEXTEND(&argv, xs.p, xs.n);
	}
	DAPUSH(&argv, nullptr);

	pid_t pid = fork();
	if (pid == -1)
		err("fork:");
	if (pid == 0) {
		for (int i = 0; i < (int)lengthof(ctx.fds); i++) {
			if (ctx.fds[i] != i) {
				close(i);
				if (dup2(ctx.fds[i], i) == -1)
					err("dup2:");
			}
		}
		execvp(argv.buf[0], argv.buf);
		err("exec:");
	}

	int ws;
	if (waitpid(pid, &ws, 0) == -1)
		err("waitpid:");

	arena_free(&a);
	return WIFEXITED(ws) ? WEXITSTATUS(ws) : UINT8_MAX + 1;
}

struct strarr
valtostrs(struct value v, alloc_fn alloc, void *ctx)
{
	struct strarr sa;

	switch (v.kind) {
	case VK_WORD:
		sa.n = 1;
		sa.p = alloc(ctx, nullptr, 0, 1, sizeof(char *), alignof(char *));
		sa.p[0] = alloc(ctx, nullptr, 0, v.w.len + 1, sizeof(char),
		                alignof(char));
		((char *)memcpy(sa.p[0], v.w.p, v.w.len))[v.w.len] = '\0';
		break;
	default:
		unreachable();
	}

	return sa;
}
