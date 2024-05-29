#include <sys/wait.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <alloc.h>
#include <dynarr.h>
#include <errors.h>
#include <macros.h>

#include "builtin.h"
#include "exec.h"
#include "parser.h"

struct strs {
	dafields(char *)
};

struct strarr {
	char **p;
	size_t n;
};

struct pload {
	struct unit u;
	struct ctx ctx;
};

struct cmpnd_pld {
	struct cmpnd cmpnd;
	struct ctx ctx;
};

static int exec_stmt(struct stmt, struct ctx);
static int exec_andor(struct andor, struct ctx);
static int exec_pipe(struct pipe, struct ctx);
static int exec_unit(struct unit, struct ctx);
static int exec_cmpnd(struct cmpnd, struct ctx);
static int exec_cmd(struct cmd, struct ctx);

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
	if (ao.r == nullptr)
		return EXIT_SUCCESS;

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

	pid_t *pids = arena_new(ctx.a, pid_t, p.len);
	if (pids == nullptr)
		err("arena_new:");

	int fds[2];
	enum {
		R,
		W,
	};

	for (size_t i = 0; i < p.len; i++) {
		struct ctx nctx = ctx;
		if (i > 0)
			nctx.fds[STDIN_FILENO] = fds[R];
		if (i < p.len - 1) {
			if (pipe(fds) == -1)
				err("pipe:");
			nctx.fds[STDOUT_FILENO] = fds[W];
		}

		pid_t pid = fork();
		if (pid == -1)
			err("fork:");
		if (pid == 0)
			exit(exec_unit(p.buf[i], nctx));

		pids[i] = pid;
		if (i < p.len - 1)
			close(nctx.fds[STDOUT_FILENO]);
		if (i > 0)
			close(nctx.fds[STDIN_FILENO]);
	}

	int ret;
	for (size_t i = 0; i < p.len; i++) {
		int ws;
		if (waitpid(pids[i], &ws, 0) == -1)
			err("waitpid:");
		ret = WIFEXITED(ws) ? WEXITSTATUS(ws) : UINT8_MAX + 1;
	}
	return ret;
}

int
exec_unit(struct unit u, struct ctx ctx)
{
	switch (u.kind) {
	case UK_CMD:
		return exec_cmd(u.c, ctx);
	case UK_CMPND:
		return exec_cmpnd(u.cp, ctx);
	}
	unreachable();
}

int
exec_cmpnd(struct cmpnd cp, struct ctx ctx)
{
	int ret = EXIT_SUCCESS;
	da_foreach (cp, stmt) {
		if ((ret = exec_stmt(*stmt, ctx)) != EXIT_SUCCESS)
			break;
	}
	return ret;
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

	builtin_fn bltn = lookup_builtin(argv.buf[0]);
	if (bltn != nullptr) {
		int ret = bltn(argv.buf, argv.len, ctx);
		arena_free(&a);
		return ret;
	}

	pid_t pid = fork();
	if (pid == -1)
		err("fork:");
	if (pid != 0) {
		arena_free(&a);

		int ws;
		if (waitpid(pid, &ws, 0) == -1)
			err("waitpid:");
		return WIFEXITED(ws) ? WEXITSTATUS(ws) : UINT8_MAX + 1;
	}

	for (int i = 0; i < (int)lengthof(ctx.fds); i++) {
		if (ctx.fds[i] != i) {
			close(i);
			if (dup2(ctx.fds[i], i) == -1)
				err("dup2:");
			close(ctx.fds[i]);
		}
	}

	DAPUSH(&argv, nullptr);
	execvp(argv.buf[0], argv.buf);
	err("exec:");
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
