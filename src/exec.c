#include <sys/msg.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <alloc.h>
#include <dynarr.h>
#include <errors.h>
#include <macros.h>
#include <unicode/string.h>

#include "bigint.h"
#include "builtin.h"
#include "exec.h"
#include "parser.h"
#include "symtab.h"
#include "vartab.h"

typedef int mqd_T;

struct strs {
	dafields(char *);
};

struct strarr {
	char **p;
	size_t n;
};

struct thrd_pload {
	mqd_T mqd;
	struct unit u;
	struct ctx ctx;
};

struct mqmsg {
	long mtype;
	int fds[FDCNT];
};

static int execstmt(struct stmt, struct ctx);
static int execandor(struct andor, struct ctx);
static int execpipe(struct pipe, struct ctx);
static int execunit(struct unit, struct ctx, mqd_T);
static int execcmpnd(struct cmpnd, struct ctx);
static int execcmd(struct cmd, struct ctx, mqd_T);
static void *thrdcb(void *);

static struct strarr valtostrs(struct value, alloc_fn, void *);
static void *memcpyz(void *restrict, const void *restrict, size_t);

constexpr size_t U64_STR_MAX = 21;
struct symtab symboltable;

void
shellinit(void)
{
	struct u8view k;
	struct vartab *vt;

	vt = symtabadd(&symboltable, U8("shell"), mkvartab());

	k.p = bufalloc(nullptr, U64_STR_MAX, sizeof(char));
	k.len = snprintf((char *)k.p, U64_STR_MAX, "%ld", (long)getpid());
	vartabadd(vt, U8("pid"), k);

	k.p = bufalloc(nullptr, U64_STR_MAX, sizeof(char));
	k.len = snprintf((char *)k.p, U64_STR_MAX, "%ld", (long)getppid());
	vartabadd(vt, U8("ppid"), k);

	if (setenv("SHELL", "Andy", 1) == -1)
		warn("setenv: SHELL=Andy:");
}

int
execprog(struct program p, struct ctx ctx)
{
	da_foreach (p, e) {
		int ret = execstmt(*e, ctx);
		if (ret != EXIT_SUCCESS)
			return ret;
	}
	return EXIT_SUCCESS;
}

int
execstmt(struct stmt stmt, struct ctx ctx)
{
	switch (stmt.kind) {
	case SK_ANDOR:
		return execandor(stmt.ao, ctx);
	}
	unreachable();
}

int
execandor(struct andor ao, struct ctx ctx)
{
	if (ao.r == nullptr)
		return EXIT_SUCCESS;

	int ret;
	switch (ao.op) {
	case '&':
		ret = execandor(*ao.l, ctx);
		if (ret == EXIT_SUCCESS)
			ret = execpipe(*ao.r, ctx);
		break;
	case '|':
		ret = execandor(*ao.l, ctx);
		if (ret != EXIT_SUCCESS)
			ret = execpipe(*ao.r, ctx);
		break;
	default:
		return execpipe(*ao.r, ctx);
	}

	return ret;
}

int
execpipe(struct pipe p, struct ctx ctx)
{
	ASSUME(p.len > 0);

	if (p.len == 1)
		return execunit(p.buf[0], ctx, -1);

	pthread_t *thrds = arena_new(ctx.a, pthread_t, p.len);
	struct thrd_pload *ploads = arena_new(ctx.a, struct thrd_pload, p.len);
	if (thrds == nullptr || ploads == nullptr)
		err("arena_new:");

	int fds[2];
	enum {
		R,
		W,
	};

	mqd_T mqd = msgget(IPC_PRIVATE, 0666);
	if (mqd == (mqd_T)-1)
		err("msgget:");

	for (size_t i = 0; i < p.len; i++) {
		struct ctx nctx = ctx;
		if (i > 0)
			nctx.fds[STDIN_FILENO] = fds[R];
		if (i < p.len - 1) {
#if __linux__
			if (pipe2(fds, O_CLOEXEC) == -1)
				err("pipe:");
#else
			if (pipe(fds) == -1)
				err("pipe:");

			for (int i = 0; i < (int)lengthof(fds); i++) {
				int flags = fcntl(fds[i], F_GETFD);
				if (flags == -1)
					err("fcntl: F_GETFD");
				if (fcntl(fds[i], F_SETFD, flags | FD_CLOEXEC) == -1)
					err("fcntl: F_SETFD");
			}
#endif
			nctx.fds[STDOUT_FILENO] = fds[W];
		}

		ploads[i].u = p.buf[i];
		ploads[i].ctx = nctx;
		ploads[i].mqd = mqd;
		errno = pthread_create(thrds + i, nullptr, thrdcb, ploads + i);
		if (errno != 0)
			err("pthread_create:");
	}

	for (size_t i = 0; i < p.len; i++) {
		struct mqmsg msg;
		if (msgrcv(mqd, &msg, sizeof(msg.fds), 0, 0) == -1)
			err("msgrcv:");
		for (int j = 0; j < FDCNT; j++) {
			if (msg.fds[j] != ctx.fds[j])
				close(msg.fds[j]);
		}
	}

	msgctl(mqd, IPC_RMID, nullptr);

	uintptr_t ret = 0;
	for (size_t i = 0; i < p.len; i++) {
		if ((errno = pthread_join(thrds[i], (void **)&ret)) != 0)
			err("pthread_join:");
	}
	return (int)ret;
}

int
execunit(struct unit u, struct ctx ctx, mqd_T mqd)
{
	int ret;
	switch (u.kind) {
	case UK_CMD:
		ret = execcmd(u.c, ctx, mqd);
		break;
	case UK_CMPND:
		ret = execcmpnd(u.cp, ctx);
		break;
	}
	if (!u.neg)
		return ret;
	return ret == EXIT_SUCCESS ? EXIT_FAILURE : EXIT_SUCCESS;
}

int
execcmpnd(struct cmpnd cp, struct ctx ctx)
{
	int ret = EXIT_SUCCESS;
	da_foreach (cp, stmt) {
		if ((ret = execstmt(*stmt, ctx)) != EXIT_SUCCESS)
			break;
	}
	return ret;
}

int
execcmd(struct cmd c, struct ctx ctx, mqd_T mqd)
{
	int ret;
	arena a = mkarena(0);
	struct arena_ctx a_ctx = {.a = ctx.a = &a};
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
		if (mqd != -1) {
			struct mqmsg msg = {.mtype = 1};
			for (int i = 0; i < FDCNT; i++)
				msg.fds[i] = ctx.fds[i];
			if (msgsnd(mqd, &msg, sizeof(msg.fds), 0) == -1)
				err("msgsnd:");
		}
		goto out;
	}

	pid_t pid = fork();
	if (pid == -1)
		err("fork:");

	if (pid == 0) {
		for (int i = 0; i < FDCNT; i++) {
			if (ctx.fds[i] != i) {
				close(i);
				if (dup2(ctx.fds[i], i) == -1)
					err("dup2:");
			}
		}

		DAPUSH(&argv, nullptr);
		execvp(argv.buf[0], argv.buf);
		err("exec: %s:", argv.buf[0]);
	}

	if (mqd != -1) {
		struct mqmsg msg = {.mtype = 1};
		for (int i = 0; i < FDCNT; i++)
			msg.fds[i] = ctx.fds[i];
		if (msgsnd(mqd, &msg, sizeof(msg.fds), 0) == -1)
			err("msgsnd:");
	}

	int ws;
	if (waitpid(pid, &ws, 0) == -1)
		err("waitpid:");
	ret = WIFEXITED(ws) ? WEXITSTATUS(ws) : UINT8_MAX + 1;

out:
	arena_free(&a);
	return ret;
}

void *
thrdcb(void *_pload)
{
	struct thrd_pload *pload = _pload;
	uintptr_t ret = execunit(pload->u, pload->ctx, pload->mqd);
	return (void *)ret;
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
		memcpyz(sa.p[0], v.w.p, v.w.len);
		break;
	case VK_ENV: {
		struct u8view sv = {.len = v.w.len + 1};
		sv.p = alloc(ctx, nullptr, 0, sv.len, sizeof(char), alignof(char));
		memcpyz((char8_t *)sv.p, v.w.p, v.w.len);
		sv.p = ucsnorm(&sv.len, sv, alloc, ctx, NF_NFC);

		char *s = getenv(sv.p);
		if (s != nullptr && *s != 0) {
			size_t len = strlen(s);
			sa.n = 1;
			sa.p = alloc(ctx, nullptr, 0, 1, sizeof(char *), alignof(char *));
			sa.p[0] = alloc(ctx, nullptr, 0, len + 1, sizeof(char),
			                alignof(char));
			memcpyz(sa.p[0], s, len);
		} else {
			sa.n = 0;
			sa.p = nullptr;
		}
		break;
	}
	case VK_VAR: {
		struct u8view sym;
		sym.p = ucsnorm(&sym.len, v.v.ident, alloc, ctx, NF_NFC);

		struct vartab *vt = symtabget(symboltable, sym);
		if (vt == nullptr || vt->len == 0) {
			sa.n = 0;
			goto var_out;
		}

		if (v.v.len != 0) {
			sa.n = 0;
			sa.p = nullptr;

			da_foreach (v.v, _v) {
				struct strarr xs = valtostrs(*_v, alloc, ctx);
				for (size_t i = 0; i < xs.n; i++) {
					struct u8view k = {xs.p[i], strlen(xs.p[i])};
					struct u8view *v = vartabget(*vt, k);
					if (v == nullptr)
						continue;

					sa.p = alloc(ctx, sa.p, sa.n, sa.n + 1, sizeof(char *),
					             alignof(char *));
					sa.p[sa.n] = alloc(ctx, nullptr, 0, v->len + 1,
					                   sizeof(char), alignof(char));
					memcpyz(sa.p[sa.n], v->p, v->len);
					sa.n++;
				}
			}
		} else {
			sa.n = vt->len;
			sa.p = alloc(ctx, nullptr, 0, sa.n, sizeof(char *),
			             alignof(char *));

			size_t i = 0;
			da_foreach (vt->numeric, k) {
				struct u8view *v = vartabget(*vt, *k);
				sa.p[i] = alloc(ctx, nullptr, 0, v->len + 1, sizeof(char),
				                alignof(char));
				memcpyz(sa.p[i++], v->p, v->len);
			}

			for (size_t j = 0; j < vt->cap; j++) {
				da_foreach (vt->bkts[j], kv) {
					if (isbigint(kv->k))
						continue;
					sa.p[i] = alloc(ctx, nullptr, 0, kv->v.len + 1,
					                sizeof(char), alignof(char));
					memcpyz(sa.p[i++], kv->v.p, kv->v.len);
				}
			}
		}

var_out:
		alloc(ctx, (void *)sym.p, sym.len, 0, sizeof(char8_t),
		      alignof(char8_t));
		break;
	}
	case VK_VARL: {
		sa.n = 1;
		sa.p = alloc(ctx, nullptr, 0, 1, sizeof(char *), alignof(char *));

		struct u8view sym;
		sym.p = ucsnorm(&sym.len, v.v.ident, alloc, ctx, NF_NFC);

		struct vartab *vt = symtabget(symboltable, sym);
		if (vt == nullptr || vt->len == 0) {
			sa.p[0] = "0";
			break;
		}

		sa.p[0] = alloc(ctx, nullptr, 0, U64_STR_MAX, sizeof(char),
		                alignof(char));

		size_t cnt = v.v.len == 0 ? vt->len : 0;
		da_foreach (v.v, _v) {
			struct strarr xs = valtostrs(*_v, alloc, ctx);
			for (size_t i = 0; i < xs.n; i++) {
				struct u8view k = {xs.p[i], strlen(xs.p[i])};
				if (vartabget(*vt, k) != nullptr)
					cnt++;
			}
		}

		snprintf(sa.p[0], U64_STR_MAX, "%zu", cnt);
		break;
	}
	case VK_SEL:
		sa.n = 0;
		sa.p = nullptr;
		da_foreach (v.l, _v) {
			struct strarr _sa = valtostrs(*_v, alloc, ctx);
			if (_sa.n != 0) {
				sa = _sa;
				break;
			}
		}
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

void *
memcpyz(void *restrict dst, const void *restrict src, size_t n)
{
	((char *)memcpy(dst, src, n))[n] = 0;
	return dst;
}
