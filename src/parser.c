#include <setjmp.h>

#include <alloc.h>
#include <errors.h>

#include "lexer.h"
#include "parser.h"

#define EAT (void)lexnext(p.l)

static struct stmt parse_stmt(struct parser);
static struct andor parse_andor(struct parser);
static struct pipe parse_pipe(struct parser);
static struct unit parse_unit(struct parser);
static struct cmd parse_cmd(struct parser);
static struct value parse_value(struct parser);

struct program *
parse_program(struct parser p)
{
	struct arena_ctx ctx = {.a = p.a};
	struct program *prog = arena_new(p.a, struct program, 1);
	if (prog == nullptr)
		err("arena_new:");

	prog->alloc = alloc_arena;
	prog->ctx = &ctx;

	for (;;) {
		struct stmt stmt = parse_stmt(p);
		DAPUSH(prog, stmt);

		struct lextok t = lexnext(p.l);
		if (t.kind < _LTK_TERM) {
			warn("%s:%tu: encountered unexpected token", p.l->file,
			     t.sv.p - p.l->base);
			longjmp(*p.err, 1);
		}
		while ((t = lexpeek(p.l)).kind > _LTK_TERM) {
			if (t.kind == LTK_EOF)
				goto out;
			EAT;
		}
	}

out:
	return prog;
}

static struct stmt
parse_stmt(struct parser p)
{
	struct stmt stmt = {
		.kind = SK_ANDOR,
		.ao = parse_andor(p),
	};
	return stmt;
}

static struct andor
parse_andor(struct parser p)
{
	struct andor ao = {};

	struct pipe pipe = parse_pipe(p);
	if (pipe.len == 0)
		return ao;
	if ((ao.r = arena_new(p.a, struct pipe, 1)) == nullptr)
		err("arena_new:");
	*ao.r = pipe;

	for (;;) {
		while (lexpeek(p.l).kind == LTK_NL)
			EAT;

		char op = 0;
		struct lextok t = lexpeek(p.l);
		if (t.kind == LTK_LAND) {
			op = '&';
			EAT;
		} else if (t.kind == LTK_LOR) {
			op = '|';
			EAT;
		} else
			break;

		struct andor *l;
		if ((l = arena_new(p.a, struct andor, 1)) == nullptr)
			err("arena_new:");
		*l = ao;
		ao = (struct andor){
			.op = op,
			.l = l,
		};

		while (lexpeek(p.l).kind == LTK_NL)
			EAT;

		pipe = parse_pipe(p);
		if (pipe.len == 0) {
			warn("%s:%tu: invalid right-hand side to ‘%.*s’ operator",
			     p.l->file, p.l->cur.sv.p - p.l->base, SV_PRI_ARGS(t.sv));
			longjmp(*p.err, 1);
		}
		if ((ao.r = arena_new(p.a, struct pipe, 1)) == nullptr)
			err("arena_new:");
		*ao.r = pipe;
	}

	return ao;
}

static struct pipe
parse_pipe(struct parser p)
{
	struct arena_ctx ctx = {.a = p.a};
	struct pipe pipe = {
		.alloc = alloc_arena,
		.ctx = &ctx,
	};

	struct unit u = parse_unit(p);
	if (u.kind == -1)
		return pipe;
	DAPUSH(&pipe, u);

	for (;;) {
		while (lexpeek(p.l).kind == LTK_NL)
			EAT;

		if (lexpeek(p.l).kind != LTK_PIPE)
			break;
		EAT;

		while (lexpeek(p.l).kind == LTK_NL)
			EAT;

		u = parse_unit(p);
		if (u.kind == -1) {
			warn("%s:%tu: invalid right-hand side to ‘%.*s’ operator",
			     p.l->file, p.l->cur.sv.p - p.l->base, 1, "|");
			longjmp(*p.err, 1);
		}
		DAPUSH(&pipe, u);
	}

	return pipe;
}

static struct unit
parse_unit(struct parser p)
{
	struct unit u = {
		.kind = UK_CMD,
		.c = parse_cmd(p),
	};
	if (u.c.len == 0)
		u.kind = -1;
	return u;
}

static struct cmd
parse_cmd(struct parser p)
{
	struct arena_ctx ctx = {.a = p.a};
	struct cmd cmd = {
		.alloc = alloc_arena,
		.ctx = &ctx,
	};

	for (;;) {
		struct value v = parse_value(p);
		if (v.kind == -1)
			break;
		DAPUSH(&cmd, v);
	}

	return cmd;
}

static struct value
parse_value(struct parser p)
{
	struct value v = {
		.kind = VK_WORD,
	};
	struct lextok t = lexpeek(p.l);
	if (t.kind != LTK_WORD)
		v.kind = -1;
	else {
		v.w = t.sv;
		EAT;
	}
	return v;
}
