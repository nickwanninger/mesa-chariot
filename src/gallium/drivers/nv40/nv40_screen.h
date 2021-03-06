#ifndef __NV40_SCREEN_H__
#define __NV40_SCREEN_H__

#include "pipe/p_screen.h"
#include "nv04/nv04_surface_2d.h"

struct nv40_screen {
	struct pipe_screen pipe;

	struct nouveau_winsys *nvws;

	unsigned cur_pctx;

	/* HW graphics objects */
	struct nv04_surface_2d *eng2d;
	struct nouveau_grobj *curie;
	struct nouveau_notifier *sync;

	/* Query object resources */
	struct nouveau_notifier *query;
	struct nouveau_resource *query_heap;

	/* Vtxprog resources */
	struct nouveau_resource *vp_exec_heap;
	struct nouveau_resource *vp_data_heap;

	/* Current 3D state of channel */
	struct nouveau_stateobj *state[NV40_STATE_MAX];
};

static INLINE struct nv40_screen *
nv40_screen(struct pipe_screen *screen)
{
	return (struct nv40_screen *)screen;
}

void
nv40_screen_init_transfer_functions(struct pipe_screen *pscreen);

#endif
