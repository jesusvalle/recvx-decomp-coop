#include "../../../ps2/veronica/prog/coop.h"

#ifdef COOP

#include <stdio.h>
#include "../../../ps2/veronica/prog/pad.h"
#include "../../../ps2/veronica/prog/player.h"
#include "../../../ps2/veronica/prog/MdlPut.h"
#include "../../../ps2/veronica/prog/Motion.h"
#include "../../../ps2/veronica/prog/effect.h"
#include "../../../ps2/veronica/prog/hitchk.h"
#include "../../../ps2/veronica/prog/playpch.h"
#include "../../../ps2/veronica/prog/ps2_NaMath.h"
#include "../../../ps2/veronica/prog/macros.h"
#include "../../../ps2/veronica/prog/njplus.h"
#include "../../../ps2/veronica/prog/pwksub.h"
#include "../../../ps2/veronica/prog/main.h"

BH_PWORK ply2 __attribute__((aligned(64)));

static COOP_PAD coop_pad2;
static int coop_enabled;
static unsigned char* coop_exp0;
static unsigned char* coop_exp1;
static unsigned char* coop_pool;
static ML_WORK coop_mdl[16];
static int coop_mdl_n;
static int coop_mlw_idx;
static int coop_need_init;

extern const float PlyInfo[4][2];
extern const char PlyFlip[23];
extern CPCL PlyCapColTab[18];

typedef struct COOP_SAVE
{
    unsigned int st_flg;
    unsigned int cb_flg;
    unsigned int gm_flg;
    unsigned int pt_flg;
    unsigned int flr_idx;
    unsigned int etc_idx;
    ATR_WORK* pl_htp;
    DOOR_WORK door;
    CAM_WORK cam;
} COOP_SAVE;

static COOP_SAVE coop_save;
static int coop_hidden;
static int coop_shadow = -1;

static void coopSwapPad(COOP_PAD* p)
{
    COOP_PAD t;

    t.on = sys->pad_on;
    t.oncpy = sys->pad_oncpy;
    t.ps = sys->pad_ps;
    t.rs = sys->pad_rs;
    t.old = sys->pad_old;
    t.onb = sys->pad_onb;
    t.psb = sys->pad_psb;
    t.oldb = sys->pad_oldb;
    t.ax = sys->pad_ax;
    t.ay = sys->pad_ay;
    t.dx = sys->pad_dx;
    t.dy = sys->pad_dy;
    t.ar = sys->pad_ar;
    t.al = sys->pad_al;

    sys->pad_on = p->on;
    sys->pad_oncpy = p->oncpy;
    sys->pad_ps = p->ps;
    sys->pad_rs = p->rs;
    sys->pad_old = p->old;
    sys->pad_onb = p->onb;
    sys->pad_psb = p->psb;
    sys->pad_oldb = p->oldb;
    sys->pad_ax = p->ax;
    sys->pad_ay = p->ay;
    sys->pad_dx = p->dx;
    sys->pad_dy = p->dy;
    sys->pad_ar = p->ar;
    sys->pad_al = p->al;

    *p = t;
}

void coopSetPad2(void)
{
    int port_bak;
    const PDS_PERIPHERAL* per_bak;

    if (((sys->ss_flg & 0xC00000)) || (!(sys->sp_flg & 0x20)))
    {
        coop_pad2.on = coop_pad2.oncpy = coop_pad2.ps = coop_pad2.rs = coop_pad2.old = 0;
        return;
    }

    coopSwapPad(&coop_pad2);

    port_bak = pd_port;
    per_bak = sys->p1per;

    pd_port = 1;

    bhSetPad();

    pd_port = port_bak;
    sys->p1per = per_bak;

    coopSwapPad(&coop_pad2);

    coop_pad2.on &= COOP_P2_PAD_MASK;
    coop_pad2.oncpy &= COOP_P2_PAD_MASK;
    coop_pad2.ps &= COOP_P2_PAD_MASK;
    coop_pad2.rs &= COOP_P2_PAD_MASK;
    coop_pad2.old &= COOP_P2_PAD_MASK;
}

void coopInitMemory(void)
{
    coop_enabled = 0;
    coop_mdl_n = 0;

    coop_exp0 = bhGetFreeMemory(sizeof(EXP_WORK), 32);
    coop_exp1 = bhGetFreeMemory(124, 32);
    coop_pool = bhGetFreeMemory(COOP_CLONE_POOL_SIZE, 64);

    if ((coop_exp0 == NULL) || (coop_exp1 == NULL) || (coop_pool == NULL))
    {
        coop_pool = NULL;

        printf("[COOP] sin memoria para P2: desactivado\n");
        return;
    }

    printf("[COOP] memoria P2 reservada (pool %d)\n", COOP_CLONE_POOL_SIZE);
}

void coopCloneModel(void)
{
    int i;
    int k;
    int n;
    int used;
    int size;
    NJS_CNK_OBJECT* src;
    NJS_CNK_OBJECT* dst;
    O_WORK* ow;

    coop_enabled = 0;

    if (coop_pool == NULL)
    {
        return;
    }

    used = 0;

    coop_mdl_n = ply.mdl_n;

    for (i = 0; i < ply.mdl_n; i++)
    {
        coop_mdl[i] = ply.mdl[i];

        n = ply.mdl[i].obj_num;

        if ((ply.mdl[i].objP == NULL) || (ply.mdl[i].owP == NULL) || (n <= 0))
        {
            continue;
        }

        size = ALIGN_UP(n * (int)sizeof(NJS_CNK_OBJECT), 64) + ALIGN_UP(n * (int)sizeof(O_WORK), 64);

        if ((used + size) > COOP_CLONE_POOL_SIZE)
        {
            printf("[COOP] pool de clonado insuficiente (%d > %d): P2 desactivado\n", used + size, COOP_CLONE_POOL_SIZE);
            return;
        }

        src = ply.mdl[i].objP;
        dst = (NJS_CNK_OBJECT*)&coop_pool[used];

        npCopyMemory((unsigned char*)dst, (unsigned char*)src, n * sizeof(NJS_CNK_OBJECT));

        for (k = 0; k < n; k++)
        {
            if ((dst[k].child != NULL) && (dst[k].child >= src) && (dst[k].child < &src[n]))
            {
                dst[k].child = &dst[dst[k].child - src];
            }

            if ((dst[k].sibling != NULL) && (dst[k].sibling >= src) && (dst[k].sibling < &src[n]))
            {
                dst[k].sibling = &dst[dst[k].sibling - src];
            }
        }

        used += ALIGN_UP(n * (int)sizeof(NJS_CNK_OBJECT), 64);

        ow = (O_WORK*)&coop_pool[used];

        npCopyMemory((unsigned char*)ow, (unsigned char*)ply.mdl[i].owP, n * sizeof(O_WORK));

        used += ALIGN_UP(n * (int)sizeof(O_WORK), 64);

        coop_mdl[i].objP = dst;
        coop_mdl[i].owP = ow;
    }

    coop_mlw_idx = (ply.mlwP != NULL) ? (ply.mlwP - ply.mdl) : 0;

    coop_enabled = 1;
    coop_need_init = 1;

    printf("[COOP] clon de P1: %d modelos, %d bytes\n", coop_mdl_n, used);
}

static void coopBegin(void)
{
    coop_save.st_flg = sys->st_flg;
    coop_save.cb_flg = sys->cb_flg;
    coop_save.gm_flg = sys->gm_flg;
    coop_save.pt_flg = sys->pt_flg;
    coop_save.flr_idx = sys->flr_idx;
    coop_save.etc_idx = sys->etc_idx;
    coop_save.pl_htp = sys->pl_htp;
    coop_save.door = sys->door;
    coop_save.cam = cam;

    coopSwapPad(&coop_pad2);

    plp = &ply2;
}

static void coopEnd(void)
{
    plp = &ply;

    coopSwapPad(&coop_pad2);

    sys->st_flg = coop_save.st_flg;
    sys->cb_flg = coop_save.cb_flg;
    sys->gm_flg = coop_save.gm_flg;
    sys->pt_flg = coop_save.pt_flg;
    sys->flr_idx = coop_save.flr_idx;
    sys->etc_idx = coop_save.etc_idx;
    sys->pl_htp = coop_save.pl_htp;
    sys->door = coop_save.door;
    cam = coop_save.cam;
}

static int coopHideCondition(void)
{
    if ((ply.stflg & 0x1000000) || (sys->cb_flg & 0x5) || (ply.mode0 == 7))
    {
        return 1;
    }

    return 0;
}

static int coopFreezeCondition(void)
{
    if ((sys->st_flg & 0x200) || (!(sys->sp_flg & 0x1)))
    {
        return 1;
    }

    return 0;
}

/* Demo de atracción: P2 no existe (sus rand() desincronizarían la grabación). */
static int coopDemo(void)
{
    if ((sys->ss_flg & 0xC00000))
    {
        return 1;
    }

    return 0;
}

static void coopPlaceNearP1(void)
{
    EXP_WORK* e;
    NJS_POINT3 from;
    NJS_POINT3 to;
    float s;
    float c;
    float d;

    e = (EXP_WORK*)ply2.exp0;

    njSinCos(ply.ay, &s, &c);

    d = (ply.ar + ply2.ar) * 1.25f;

    from.x = ply.px;
    from.y = ply.py;
    from.z = ply.pz;

    to.x = ply.px - (c * d);
    to.y = ply.py;
    to.z = ply.pz + (s * d);

    ply2.flr_no = ply.flr_no;

    ply2.px = to.x;
    ply2.py = to.y;
    ply2.pz = to.z;

    if (bhCheckWallEx(&ply2, &to, &from, ply2.ar, ply2.ah) != 0)
    {
        to = from;
    }

    ply2.px = ply2.gpx = ply2.pxb = e->spx = e->plx = e->nlxb = to.x;
    ply2.py = ply2.gpy = ply2.pyb = e->spy = e->ply = e->nlyb = to.y;
    ply2.pz = ply2.gpz = ply2.pzb = e->spz = e->plz = e->nlzb = to.z;

    e->bpx = e->bpxb = to.x;
    e->bpy = e->bpyb = to.y + 12.5f;
    e->bpz = e->bpzb = to.z;

    e->arn = 0;
    e->arp = 0;

    ply2.ax = ply2.az = 0;
    ply2.ay = ply2.ayb = ply.ay;
    ply2.spd = 0;

    ply2.stflg &= ~0x1;
    ply2.psh_ct = 0;

    ply2.hokan_count = 0;
    ply2.hokan_rate = 0;
    ply2.frm_mode = 0;
    ply2.frm_no = 0;
    ply2.mtn_no = PlMtnAct[0][0][0];
    ply2.mtn_add = 65536;
    ply2.mtn_md = 0;
    ply2.mtn_tp = (unsigned char*)PlyFlip;
    ply2.mnwP = ply2.mnwPb;

    bhSetMotion(&ply2, (int)ply2.mtn_add, ply2.mtn_md, ply2.mtn_tp);

    *(int*)&ply2.mode0 = 1;

    bhCalcModel(&ply2);
}

void coopRoomStart(void)
{
    int i;

    coop_hidden = 1;
    coop_need_init = 0;

    if ((coop_enabled == 0) || (coopDemo() != 0))
    {
        return;
    }

    if ((coop_shadow >= 0) && (eff[coop_shadow].lkwkp == (unsigned char*)&ply2) && (eff[coop_shadow].flg & 0x1))
    {
        eff[coop_shadow].flg = 0;
    }

    npSetMemory((unsigned char*)&ply2, sizeof(BH_PWORK), 0);
    npSetMemory(coop_exp0, sizeof(EXP_WORK), 0);
    npSetMemory(coop_exp1, 124, 0);

    for (i = 0; i < coop_mdl_n; i++)
    {
        ply2.mdl[i] = coop_mdl[i];
    }

    for (i = 0; i < 16; i++)
    {
        ply2.skp[i] = ply.skp[i];
        ply2.mbp[i] = ply.mbp[i];
        ply2.txp[i] = ply.txp[i];
    }

    ply2.mdl_n = coop_mdl_n;
    ply2.mdl_no = 0;
    ply2.mlwP = &ply2.mdl[coop_mlw_idx];
    ply2.mlwP->texP = ply2.txp[0];
    ply2.mtx = (float(*)[16])ply2.mtxbuf;

    ply2.exp0 = coop_exp0;
    ply2.exp1 = coop_exp1;
    ply2.exp3 = NULL;

    ply2.flg = 0x119;
    ply2.mdflg = 0x20;
    ply2.stflg = 0x40000000 | 0x1000000;

    ply2.ar = PlyInfo[sys->ply_id][0];
    ply2.ah = PlyInfo[sys->ply_id][1];
    ply2.car = PlyInfo[sys->ply_id][0] - 1.0f;
    ply2.cah = PlyInfo[sys->ply_id][1] - 1.0f;

    ply2.sx = ply2.sxb = 1.0f;
    ply2.sy = ply2.syb = 1.0f;
    ply2.sz = ply2.szb = 1.0f;

    ply2.hp = COOP_P2_HP;
    ply2.wpnr_no = 0;
    ply2.wpnl_no = 0;

    ply2.clp_jno[0] = 0;
    ply2.clp_jno[1] = 5;
    ply2.clp_jno[2] = 9;
    ply2.clp_jno[3] = 13;
    ply2.clp_jno[4] = -1;

    ply2.cpcl = PlyCapColTab;

    ((int*)ply2.exp1)[0] = 1;
    ((short*)ply2.exp1)[34] = -1;

    ply2.mnwP = ply2.mnwPb = ply.mnwPb;

    ply2.mlwP->owP[7].flg |= 0x8;
    ply2.mlwP->owP[11].flg |= 0x8;

    PlyPchInit(&ply2);

    coopBegin();
    coopPlaceNearP1();
    coopEnd();

    coop_shadow = bhSetShadow(NULL, (unsigned char*)&ply2, 1, 4.5f, 4.0f, 3.5f);

    if (coop_shadow >= 0)
    {
        ply2.flg |= 0x800;
    }

    printf("[COOP] P2 en sala %d-%d (%d, %d, %d)\n", sys->stg_no, sys->rom_no, (int)ply2.px, (int)ply2.py, (int)ply2.pz);
}

static void coopSeparate(void)
{
    NJS_POINT3 np;
    NJS_POINT3 op;
    float dx;
    float dz;
    float d;
    float min;

    dx = ply2.px - ply.px;
    dz = ply2.pz - ply.pz;

    min = ply.ar + ply2.ar;

    d = njSqrt((dx * dx) + (dz * dz));

    if (d >= min)
    {
        return;
    }

    if (d < 0.01f)
    {
        dx = 1.0f;
        dz = 0;
        d = 1.0f;
    }

    op.x = ply2.px;
    op.y = ply2.py;
    op.z = ply2.pz;

    np.x = ply.px + (dx * (min / d));
    np.y = ply2.py;
    np.z = ply.pz + (dz * (min / d));

    bhCheckWallEx(&ply2, &np, &op, ply2.ar, ply2.ah);

    ply2.px = np.x;
    ply2.pz = np.z;

    bhCalcModel(&ply2);
}

void coopControlPlayer2(void)
{
    if ((coop_enabled == 0) || (coopDemo() != 0))
    {
        return;
    }

    if (coop_need_init != 0)
    {
        coopRoomStart();
    }

    if (coopHideCondition() != 0)
    {
        if (coop_hidden == 0)
        {
            printf("[COOP] P2 oculto (evento)\n");
        }

        coop_hidden = 1;

        ply2.stflg |= 0x1000000;

        coopBegin();
        coopPlaceNearP1();
        coopEnd();
        return;
    }

    if (coop_hidden != 0)
    {
        coop_hidden = 0;

        ply2.stflg &= ~0x1000000;

        printf("[COOP] P2 visible\n");
    }

    if (coopFreezeCondition() != 0)
    {
        return;
    }

    coopBegin();

    ply2.hp = COOP_P2_HP;
    ply2.psh_ct = 0;
    ply2.stflg &= ~0x80;

    bhControlPlayer();

    ply2.psh_ct = 0;
    ply2.stflg &= ~0x80;

    coopSeparate();

    ply2.psh_ct = 0;
    ply2.stflg &= ~0x80;

    coopEnd();
}

void coopDrawPlayer2(void)
{
    if ((coop_enabled == 0) || (coopDemo() != 0))
    {
        return;
    }

    if ((!(sys->pt_flg & 0x1)) || (ply2.stflg & 0x1000000) || (ply2.mdflg & 0x1))
    {
        return;
    }

    if (!(ply2.mdflg & 0x20))
    {
        if (bhCheckClipModel(&ply2) == 0)
        {
            bhPutModel(&ply2);
        }
    }
    else
    {
        bhPutModel(&ply2);
    }
}

#endif
