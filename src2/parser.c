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
		while (lexpeek(l).kind > _LTK_TERM)
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

	while (lexpeek(*l).kind == LTK_ARG) {
		struct lextok t = lexnext(l);
		struct value v = {
			.kind = VK_ARG,
			.arg = t.sv,
		};
		DAPUSH(&e.b, v);
	}

	return e.b.len == 0 ? (e.kind = EK_INVAL, e) : e;
}
