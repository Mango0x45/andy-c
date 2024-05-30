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
	int ret;
	switch (u.kind) {
	case UK_CMD:
		ret = exec_cmd(u.c, ctx);
		break;
	case UK_CMPND:
		ret = exec_cmpnd(u.cp, ctx);
		break;
	}
	if (!u.neg)
		return ret;
	return ret == EXIT_SUCCESS ? EXIT_FAILURE : EXIT_SUCCESS;
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
	int ret;
	arena a = mkarena(0);
	struct arena_ctx a_ctx = {.a = &a};
	struct strs argv = {
		.alloc = alloc_arena,
		.ctx = &a_ctx,
	};

	DAGROW(&argv, c.len + 1);
	da_foreach (c, v) {
		struct strarr xs = valtostrs(*v, alloc_arena, &a_ctx);
		if (xs.n > 0)
			DAEXTEND(&argv, xs.p, xs.n);
	}

	if (argv.len == 0) {
		ret = EXIT_SUCCESS;
		goto out;
	}

	builtin_fn bltn = lookup_builtin(argv.buf[0]);
	if (bltn != nullptr) {
		ret = bltn(argv.buf, argv.len, ctx);
		goto out;
	}

	pid_t pid = fork();
	if (pid == -1)
		err("fork:");
	if (pid != 0) {
		int ws;
		if (waitpid(pid, &ws, 0) == -1)
			err("waitpid:");
		ret = WIFEXITED(ws) ? WEXITSTATUS(ws) : UINT8_MAX + 1;
		goto out;
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
	err("exec: %s:", argv.buf[0]);

out:
	arena_free(&a);
	return ret;
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
	case VK_LIST:
		sa.n = 0;
		sa.p = nullptr;

		da_foreach (v.l, _v) {
			struct strarr _sa = valtostrs(*_v, alloc, ctx);
			sa.p = alloc(ctx, sa.p, sa.n, sa.n + _sa.n, sizeof(char *),
			             alignof(char *));
			for (size_t i = 0; i < _sa.n; i++)
				sa.p[sa.n + i] = _sa.p[i];
			sa.n += _sa.n;
		}
		break;
	case VK_CONCAT:
		struct strarr lhs, rhs;
		lhs = valtostrs(*v.c.l, alloc, ctx);
		rhs = valtostrs(*v.c.r, alloc, ctx);
		sa.n = lhs.n * rhs.n;
		sa.p = alloc(ctx, nullptr, 0, sa.n, sizeof(char *), alignof(char *));

		for (size_t i = 0, k = 0; i < lhs.n; i++) {
			size_t n = strlen(lhs.p[i]);
			for (size_t j = 0; j < rhs.n; j++, k++) {
				size_t m = strlen(rhs.p[j]);
				sa.p[k] = alloc(ctx, nullptr, 0, n + m + 1, sizeof(char),
				                alignof(char));
				memcpy(sa.p[k] + 0, lhs.p[i], n);
				memcpy(sa.p[k] + n, rhs.p[j], m);
				sa.p[k][n + m] = 0;
			}
		}
		break;
	default:
		unreachable();
	}

	return sa;
}
