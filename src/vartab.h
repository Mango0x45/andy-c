#ifndef ANDY_VARTAB_H
#define ANDY_VARTAB_H

#include <dynarr.h>
#include <mbstring.h>

struct vartabpair {
	struct u8view k;
	struct u8view v;
};

struct vartab {
	struct vartabbkt {
		dafields(struct vartabpair)
	} *bkts;
	size_t len, cap;
};

struct vartab mkvartab(void);
struct u8view vartabget(struct vartab, struct u8view);
void vartabadd(struct vartab *, struct u8view, struct u8view);
void vartabdel(struct vartab *, struct u8view);
void vartabfree(struct vartab);

#endif /* !ANDY_VARTAB_H */
