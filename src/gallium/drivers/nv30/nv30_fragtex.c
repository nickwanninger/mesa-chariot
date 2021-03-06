#include "nv30_context.h"
#include "nouveau/nouveau_util.h"

#define _(m,tf,ts0x,ts0y,ts0z,ts0w,ts1x,ts1y,ts1z,ts1w)                        \
{                                                                              \
  TRUE,                                                                        \
  PIPE_FORMAT_##m,                                                             \
  NV34TCL_TX_FORMAT_FORMAT_##tf,                                               \
  (NV34TCL_TX_SWIZZLE_S0_X_##ts0x | NV34TCL_TX_SWIZZLE_S0_Y_##ts0y |           \
   NV34TCL_TX_SWIZZLE_S0_Z_##ts0z | NV34TCL_TX_SWIZZLE_S0_W_##ts0w |           \
   NV34TCL_TX_SWIZZLE_S1_X_##ts1x | NV34TCL_TX_SWIZZLE_S1_Y_##ts1y |           \
   NV34TCL_TX_SWIZZLE_S1_Z_##ts1z | NV34TCL_TX_SWIZZLE_S1_W_##ts1w)            \
}

struct nv30_texture_format {
	boolean defined;
	uint	pipe;
	int     format;
	int     swizzle;
};

static struct nv30_texture_format
nv30_texture_formats[] = {
	_(A8R8G8B8_UNORM, A8R8G8B8,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(A1R5G5B5_UNORM, A1R5G5B5,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(A4R4G4B4_UNORM, A4R4G4B4,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(R5G6B5_UNORM  , R5G6B5  ,   S1,   S1,   S1,  ONE, X, Y, Z, W),
	_(L8_UNORM      , L8      ,   S1,   S1,   S1,  ONE, X, X, X, X),
	_(A8_UNORM      , L8      , ZERO, ZERO, ZERO,   S1, X, X, X, X),
	_(I8_UNORM      , L8      ,   S1,   S1,   S1,   S1, X, X, X, X),
	_(A8L8_UNORM    , A8L8    ,   S1,   S1,   S1,   S1, X, X, X, Y),
//	_(Z16_UNORM     , Z16     ,   S1,   S1,   S1,  ONE, X, X, X, X),
//	_(Z24S8_UNORM   , Z24     ,   S1,   S1,   S1,  ONE, X, X, X, X),
	_(DXT1_RGB      , DXT1    ,   S1,   S1,   S1,  ONE, X, Y, Z, W),
	_(DXT1_RGBA     , DXT1    ,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(DXT3_RGBA     , DXT3    ,   S1,   S1,   S1,   S1, X, Y, Z, W),
	_(DXT5_RGBA     , DXT5    ,   S1,   S1,   S1,   S1, X, Y, Z, W),
	{},
};

static struct nv30_texture_format *
nv30_fragtex_format(uint pipe_format)
{
	struct nv30_texture_format *tf = nv30_texture_formats;
	char fs[128];

	while (tf->defined) {
		if (tf->pipe == pipe_format)
			return tf;
		tf++;
	}

	NOUVEAU_ERR("unknown texture format %s\n", pf_name(pipe_format));
	return NULL;
}


static struct nouveau_stateobj *
nv30_fragtex_build(struct nv30_context *nv30, int unit)
{
	struct nv30_sampler_state *ps = nv30->tex_sampler[unit];
	struct nv30_miptree *nv30mt = nv30->tex_miptree[unit];
	struct pipe_texture *pt = &nv30mt->base;
	struct nv30_texture_format *tf;
	struct nouveau_stateobj *so;
	uint32_t txf, txs , txp;
	unsigned tex_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;

	tf = nv30_fragtex_format(pt->format);
	if (!tf)
		assert(0);

	txf  = tf->format;
	txf |= ((pt->last_level>0) ? NV34TCL_TX_FORMAT_MIPMAP : 0);
	txf |= log2i(pt->width[0]) << 20;
	txf |= log2i(pt->height[0]) << 24;
	txf |= log2i(pt->depth[0]) << 28;
	txf |= NV34TCL_TX_FORMAT_NO_BORDER | 0x10000;

	switch (pt->target) {
	case PIPE_TEXTURE_CUBE:
		txf |= NV34TCL_TX_FORMAT_CUBIC;
		/* fall-through */
	case PIPE_TEXTURE_2D:
		txf |= NV34TCL_TX_FORMAT_DIMS_2D;
		break;
	case PIPE_TEXTURE_3D:
		txf |= NV34TCL_TX_FORMAT_DIMS_3D;
		break;
	case PIPE_TEXTURE_1D:
		txf |= NV34TCL_TX_FORMAT_DIMS_1D;
		break;
	default:
		NOUVEAU_ERR("Unknown target %d\n", pt->target);
		return NULL;
	}

	if (!(pt->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR)) {
		txp = 0;
	} else {
		txp  = nv30mt->level[0].pitch;
		txf |= (1<<13) /*FIXME: NV34TCL_TX_FORMAT_LINEAR ? */;
	}

	txs = tf->swizzle;

	so = so_new(16, 2);
	so_method(so, nv30->screen->rankine, NV34TCL_TX_OFFSET(unit), 8);
	so_reloc (so, nv30mt->buffer, 0, tex_flags | NOUVEAU_BO_LOW, 0, 0);
	so_reloc (so, nv30mt->buffer, txf, tex_flags | NOUVEAU_BO_OR,
		  NV34TCL_TX_FORMAT_DMA0, NV34TCL_TX_FORMAT_DMA1);
	so_data  (so, ps->wrap);
	so_data  (so, NV34TCL_TX_ENABLE_ENABLE | ps->en);
	so_data  (so, txs);
	so_data  (so, ps->filt | 0x2000 /*voodoo*/);
	so_data  (so, (pt->width[0] << NV34TCL_TX_NPOT_SIZE_W_SHIFT) |
		       pt->height[0]);
	so_data  (so, ps->bcol);

	return so;
}

static boolean
nv30_fragtex_validate(struct nv30_context *nv30)
{
	struct nv30_fragment_program *fp = nv30->fragprog;
	struct nv30_state *state = &nv30->state;
	struct nouveau_stateobj *so;
	unsigned samplers, unit;

	samplers = state->fp_samplers & ~fp->samplers;
	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		so = so_new(2, 0);
		so_method(so, nv30->screen->rankine, NV34TCL_TX_ENABLE(unit), 1);
		so_data  (so, 0);
		so_ref(so, &nv30->state.hw[NV30_STATE_FRAGTEX0 + unit]);
		so_ref(NULL, &so);
		state->dirty |= (1ULL << (NV30_STATE_FRAGTEX0 + unit));
	}

	samplers = nv30->dirty_samplers & fp->samplers;
	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		so = nv30_fragtex_build(nv30, unit);
		so_ref(so, &nv30->state.hw[NV30_STATE_FRAGTEX0 + unit]);
		so_ref(NULL, &so);
		state->dirty |= (1ULL << (NV30_STATE_FRAGTEX0 + unit));
	}

	nv30->state.fp_samplers = fp->samplers;
	return FALSE;
}

struct nv30_state_entry nv30_state_fragtex = {
	.validate = nv30_fragtex_validate,
	.dirty = {
		.pipe = NV30_NEW_SAMPLER | NV30_NEW_FRAGPROG,
		.hw = 0
	}
};
