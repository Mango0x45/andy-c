#include <alloc.h>
#include <errors.h>

#include "lexer.h"
#include "parser.h"

static struct expr parse_expr(struct lexer *, arena *);

struct program *
parse_program(struct lexer l, arena *a)
{
	struct arena_ctx ctx = {.a = a};
	struct program *p = arena_new(a, struct program, 1);
	if (p == nullptr)
		err("arena_new:");

	p->alloc = alloc_arena;
	p->ctx = &ctx;

	for (;;) {
		struct expr e = parse_expr(&l, a);
		DAPUSH(p, e);

		struct lextok t = lexnext(&l);
		if (t.kind < _LTK_TERM) {
			warn("%s:%tu: expected end of expression", l.file, t.sv.p - l.base);
			return nullptr;
		}
		if (t.kind == LTK_EOF)
			break;
		while (lexpeek(&l).kind > _LTK_TERM)
			(void)lexnext(&l);
	}
	return p;
}

struct expr
parse_expr(struct lexer *l, arena *a)
{
	struct arena_ctx ctx = {.a = a};
	struct expr e = {
		.kind = EK_BASIC,
		.b.alloc = alloc_arena,
		.b.ctx = &ctx,
	};

	while (lexpeek(l).kind == LTK_ARG) {
		struct lextok t = lexnext(l);
		struct value v = {
			.kind = VK_ARG,
			.arg = t.sv,
		};
		DAPUSH(&e.b, v);
	}

	if (e.b.len == 0) {
		e.kind = EK_INVAL;
		return e;
	}

	enum lextokkind ltk = lexpeek(l).kind;
	if (ltk == LTK_PIPE || ltk == LTK_LAND || ltk == LTK_LOR) {
		(void)lexnext(l);
		struct expr cpy = e;
		e.kind = EK_BINOP;
		e.bo.kind = ltk == LTK_PIPE ? BK_PIPE
		          : ltk == LTK_LAND ? BK_AND
		                            : BK_OR;
		e.bo.lhs = arena_new(a, struct expr, 1);
		e.bo.rhs = arena_new(a, struct expr, 1);
		if (e.bo.lhs == nullptr || e.bo.rhs == nullptr)
			err("arena_new:");
		*e.bo.lhs = cpy;
		*e.bo.rhs = parse_expr(l, a);
	}

	return e;
}
