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
#include "../../../ps2/veronica/prog/dread.h"
#include "../../../ps2/veronica/prog/binfunc.h"
#include "../../../ps2/veronica/prog/ps2_texture.h"
#include "../../../ps2/veronica/prog/sdfunc.h"
#include "../../../ps2/veronica/prog/ps2_NaMem.h"
#include "../../../ps2/veronica/prog/objitm.h"
#include "../../../ps2/veronica/prog/weapon.h"
#include "../../../ps2/veronica/prog/padman.h"

BH_PWORK ply2 __attribute__((aligned(64)));

static COOP_PAD coop_pad2;
static int coop_enabled;
static unsigned char* coop_exp0;
static unsigned char* coop_exp1;
static unsigned char* coop_pool;
static ML_WORK coop_mdl[16];
static int coop_mdl_n;
static int* coop_skp[16];
static NJS_CNK_OBJECT* coop_mbp[16];
static NJS_TEXLIST* coop_txp[16];
static int coop_loaded;
static int coop_ld_mode;
static int coop_ld_file;
static unsigned char* coop_ld_buf;
static unsigned char* coop_hair_exp0;
static unsigned char* coop_hair_exp3;
static unsigned char* coop_wpn_area;
static int coop_wpn_area_size;
static O_WRK coop_wpn[2];
static int coop_wpn_ok[2];
static int coop_wpnr_no;
static O_WRK coop_hair;
static unsigned char coop_ene4[128];
static LGT_WORK coop_lgt0;
static int coop_port_bak;
static int coop_hair_ok;

extern ETTY_WORK lkmtab[2];

extern unsigned int Ps2_free_texmemsize;

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
    unsigned int mask;

    if (((sys->ss_flg & 0xC00000)) || (!(sys->sp_flg & 0x20)))
    {
        coop_pad2.on = coop_pad2.oncpy = coop_pad2.ps = coop_pad2.rs = coop_pad2.old = 0;

        coopVibStop2();
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

    mask = COOP_P2_PAD_MASK;

    /* Con armas de mira, P2 no apunta: la mira cambia la cámara y el dibujo para todos. */
    if ((WpnTab[ply2.wpnr_no].flg & 0x20))
    {
        mask &= ~0x10;
    }

    coop_pad2.on &= mask;
    coop_pad2.oncpy &= mask;
    coop_pad2.ps &= mask;
    coop_pad2.rs &= mask;
    coop_pad2.old &= mask;
}

void coopInitMemory(void)
{
    coop_enabled = 0;
    coop_loaded = 0;
    coop_ld_mode = 0;
    coop_mdl_n = 0;

    coop_exp0 = bhGetFreeMemory(sizeof(EXP_WORK), 32);
    coop_exp1 = bhGetFreeMemory(124, 32);
    coop_pool = bhGetFreeMemory(COOP_MODEL_POOL_SIZE, 64);

    if ((coop_exp0 == NULL) || (coop_exp1 == NULL) || (coop_pool == NULL) || (sys->lmmdlp == NULL) || (sys->memp > sys->endp))
    {
        coop_pool = NULL;

        printf("[COOP] sin memoria para P2: desactivado\n");
        return;
    }

    /* sys->lmmdlp (32 KB) no lo usa el juego: buffers de la coleta y huesos de las manos de P2 */
    coop_hair_exp3 = sys->lmmdlp;
    coop_hair_exp0 = &sys->lmmdlp[COOP_HAIR_EXP3_SIZE];
    coop_wpn_area = &sys->lmmdlp[COOP_HAIR_EXP3_SIZE + COOP_HAIR_EXP0_SIZE];
    coop_wpn_area_size = COOP_LMM_SIZE - (COOP_HAIR_EXP3_SIZE + COOP_HAIR_EXP0_SIZE);

    coop_enabled = 1;

    printf("[COOP] memoria P2 reservada (pool %d)\n", COOP_MODEL_POOL_SIZE);
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

/* Copia reducida de bhReadPlayerData (dread.c): solo modelos, skin, owP y texturas, en memoria de P2. */
static int coopReadPlayer2Data(unsigned char* datp)
{
    ML_WORK* mdlp;
    unsigned char* mp;
    unsigned char* tp;
    unsigned int dt0;
    unsigned int dt1;
    unsigned int need;
    int temp;
    int size;
    int i;

    coop_loaded = 0;
    coop_mdl_n = 0;

    npSetMemory((unsigned char*)coop_mdl, sizeof(coop_mdl), 0);

    for (i = 0; i < 16; i++)
    {
        coop_skp[i] = NULL;
        coop_mbp[i] = NULL;
        coop_txp[i] = NULL;
    }

    dt0 = *(unsigned int*)datp;
    datp += 4;

    if ((dt0 + 256) > COOP_MODEL_POOL_SIZE)
    {
        printf("[COOP] el modelo de P2 no cabe (%d)\n", dt0);
        return 0;
    }

    njMemCopy(coop_pool, datp, dt0);

    mp = coop_pool;
    mdlp = coop_mdl;

    while ((dt1 = *(unsigned int*)mp) != -1)
    {
        if (dt1 != 0)
        {
            mp += 4;

            if (*(unsigned int*)mp == SKIN_MAGIC)
            {
                coop_skp[coop_mdl_n] = (int*)&mp[4];
            }
            else if (coop_mdl_n < 16)
            {
                bhMlbBinRealize(mp, mdlp);

                if (coop_skp[coop_mdl_n] != NULL)
                {
                    npSkinConvert(mdlp->objP, coop_skp[coop_mdl_n]);
                }

                coop_mbp[coop_mdl_n] = mdlp->objP;
                coop_txp[coop_mdl_n] = mdlp->texP;

                mdlp++;
                coop_mdl_n++;
            }

            mp = &mp[dt1];
        }
        else
        {
            mp += 4;
        }
    }

    datp = &datp[dt0];

    dt1 = *(unsigned int*)datp;
    datp = &datp[4 + dt1];

    dt1 = *(unsigned int*)datp;
    datp = &datp[4 + dt1];

    mp = (unsigned char*)(((int)&coop_pool[dt0] + 256) & ~0xFF);

    for (i = 0; i < coop_mdl_n; i++)
    {
        size = coop_mdl[i].obj_num * sizeof(O_WORK);

        if ((mp + size) > &coop_pool[COOP_MODEL_POOL_SIZE])
        {
            printf("[COOP] los owP de P2 no caben\n");

            coop_mdl_n = 0;
            return 0;
        }

        coop_mdl[i].owP = (O_WORK*)mp;

        npSetMemory(mp, size, 0);

        mp = (unsigned char*)ALIGN_UP((int)&mp[size], 32);
    }

    need = 0;
    tp = datp;

    for (i = 0; i < coop_mdl_n; i++)
    {
        if (coop_mdl[i].texP != NULL)
        {
            temp = *(unsigned int*)tp;

            if ((temp & 0x80000000))
            {
                tp += 4;
                tp = (unsigned char*)(((int)tp + 31) & ~0x1F);
                temp &= ~0x80000000;
            }
            else
            {
                tp += 4;
            }

            need += temp;
            tp = &tp[temp];
        }
    }

    if (Ps2_free_texmemsize < need)
    {
        printf("[COOP] sin memoria de texturas para P2 (%d < %d)\n", Ps2_free_texmemsize, need);

        coop_mdl_n = 0;
        return 0;
    }

    for (i = 0; i < coop_mdl_n; i++)
    {
        if (coop_mdl[i].texP != NULL)
        {
            temp = *(unsigned int*)datp;

            if ((temp & 0x80000000))
            {
                datp += 4;
                datp = (unsigned char*)(((int)datp + 31) & ~0x1F);
                temp &= ~0x80000000;
            }
            else
            {
                datp += 4;
            }

            coop_mdl[i].flg |= 0x200;

            bhSetMemPvpTexture(coop_mdl[i].texP, datp, 0);

            datp = &datp[temp];
        }
        else
        {
            coop_mdl[i].flg &= ~0x200;
        }
    }

    coop_loaded = 1;

    printf("[COOP] P2 cargado: fichero %d, %d modelos, %d B de modelo\n", coop_ld_file, coop_mdl_n, dt0);
    return 1;
}

#ifdef COOP_TEST
/* Solo para pruebas: pone la pistola (id 5, 15 balas) en el inventario de P1 si no la tiene. */
static void coopTestGiveHandgun(void)
{
    unsigned int* pip;
    int i;

    pip = &sys->itm[sys->ply_id * 16];

    for (i = 2; i < 10; i++)
    {
        if (((pip[i] >> 16) & 0xFF) == 5)
        {
            return;
        }
    }

    for (i = 2; i < 10; i++)
    {
        if (pip[i] == 0)
        {
            pip[i] = 0x0005000F;
            return;
        }
    }
}
#endif

/* G8: paso 10 del modo 1 del cargador. Devuelve 0 mientras está ocupado y 1 al terminar (falle o no). */
int coopLoadPlayer2(void)
{
    int size;
    int st;

    switch (coop_ld_mode)
    {
    case 0:
        if ((coop_enabled == 0) || (coop_loaded != 0) || (coopDemo() != 0))
        {
            coop_ld_mode = 3;
            return 1;
        }

        if (GetReadFileStatus() != 0)
        {
            return 0;
        }

        coop_ld_file = (sys->costume == 0) ? 14 : 10;

        size = GetInsideFileSize(sys->sys_partid, coop_ld_file);

        coop_ld_buf = (unsigned char*)ALIGN_UP((int)sys->memp, 64);

        if ((size <= 0) || (&coop_ld_buf[size] > (sys->endp - COOP_PXLCONV_SIZE)))
        {
            printf("[COOP] no se puede leer el fichero %d de P2 (%d B)\n", coop_ld_file, size);

            coop_ld_mode = 3;
            return 1;
        }

        coop_ld_mode = 1;
        return 0;
    case 1:
        if (GetReadFileStatus() != 0)
        {
            return 0;
        }

        if (RequestReadInsideFile(sys->sys_partid, coop_ld_file, coop_ld_buf) != 0)
        {
            return 0;
        }

        coop_ld_mode = 2;
        return 0;
    case 2:
        st = GetReadFileStatus();

        if (st == 1)
        {
            return 0;
        }

        if (st == 0)
        {
            coopReadPlayer2Data(coop_ld_buf);

#ifdef COOP_TEST
            coopTestGiveHandgun();
#endif
        }
        else
        {
            printf("[COOP] error al leer el fichero %d de P2\n", coop_ld_file);
        }

        coop_ld_mode = 3;
        return 1;
    }

    return 1;
}

/* El fogonazo de P2 se enlaza a plp (lkflg 1), que bhControlLight resuelve con P1: se fija en la mano de P2. */
static void coopFlashToP2(void)
{
    LGT_WORK* lp;
    unsigned char* a;
    unsigned char* b;
    int i;
    int changed;

    if (rom->lgtp == NULL)
    {
        return;
    }

    lp = rom->lgtp;

    a = (unsigned char*)lp;
    b = (unsigned char*)&coop_lgt0;

    changed = 0;

    for (i = 0; i < (int)sizeof(LGT_WORK); i++)
    {
        if (a[i] != b[i])
        {
            changed = 1;
            break;
        }
    }

    if ((changed == 0) || (lp->lkflg != 1))
    {
        return;
    }

    if (lp->lkono == 0)
    {
        njCalcPoint(ply2.mtx, (NJS_POINT3*)&lp->lx, (NJS_POINT3*)&lp->px);
    }
    else
    {
        njCalcPoint(&ply2.mlwP->owP[lp->lkono].mtx, (NJS_POINT3*)&lp->lx, (NJS_POINT3*)&lp->px);
    }

    lp->lkflg = 0;
}

/* El disparo escribe en sys->obwp[0] (corredera, bombeo): durante la ventana de P2, ese objeto es el de P2. */
static void coopSwapWeaponObj(void)
{
    O_WRK* a;
    O_WRK* b;
    unsigned int f;
    unsigned char m;
    ML_WORK* w;

    if (coop_wpn_ok[0] == 0)
    {
        return;
    }

    a = sys->obwp;
    b = &coop_wpn[0];

    f = a->flg & 0xC80000;
    a->flg = (a->flg & ~0xC80000) | (b->flg & 0xC80000);
    b->flg = (b->flg & ~0xC80000) | f;

    m = a->mode0;
    a->mode0 = b->mode0;
    b->mode0 = m;

    w = a->mlwP;
    a->mlwP = b->mlwP;
    b->mlwP = w;
}

static void coopBegin(void)
{
    int i;

    coop_save.st_flg = sys->st_flg;
    coop_save.cb_flg = sys->cb_flg;
    coop_save.gm_flg = sys->gm_flg;
    coop_save.pt_flg = sys->pt_flg;
    coop_save.flr_idx = sys->flr_idx;
    coop_save.etc_idx = sys->etc_idx;
    coop_save.pl_htp = sys->pl_htp;
    coop_save.door = sys->door;
    coop_save.cam = cam;

    for (i = 0; i < 128; i++)
    {
        coop_ene4[i] = ((ene[i].flg & 0x4)) ? 1 : 0;

        ene[i].flg &= ~0x4;
    }

    if (rom->lgtp != NULL)
    {
        coop_lgt0 = rom->lgtp[0];
    }

    coop_port_bak = CurrentPortId;
    CurrentPortId = 1;

    coopSwapPad(&coop_pad2);

    plp = &ply2;
}

static void coopEnd(void)
{
    int i;

    plp = &ply;

    coopSwapPad(&coop_pad2);

    sys->st_flg = coop_save.st_flg;
    sys->cb_flg = coop_save.cb_flg;
    /* Munición compartida: si P2 vació (o recargó) el arma, P1 también lo ve. */
    sys->gm_flg = (coop_save.gm_flg & ~0x40000) | (sys->gm_flg & 0x40000);
    sys->pt_flg = coop_save.pt_flg;
    sys->flr_idx = coop_save.flr_idx;
    sys->etc_idx = coop_save.etc_idx;
    sys->pl_htp = coop_save.pl_htp;
    sys->door = coop_save.door;
    cam = coop_save.cam;

    CurrentPortId = coop_port_bak;

    for (i = 0; i < 128; i++)
    {
        if (coop_ene4[i] != 0)
        {
            ene[i].flg |= 0x4;
        }
    }

    coopFlashToP2();
}

static int coopHideCondition(void)
{
    if ((ply.stflg & 0x1000000) || (sys->cb_flg & 0x5) || (ply.mode0 == 7) || (sys->ply_id != 0))
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

/* Lo que hace bhCPM2_act_wre al bajar el arma: sin esto, stflg 0x10000 impide volver a apuntar. */
static void coopLeaveCombatP2(void)
{
    ply2.stflg &= ~0x10400;
    ply2.flg &= ~0x10000;
    ply2.at_flg = 0;
    ply2.mtn_add = 0;
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

    coopLeaveCombatP2();

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

static void coopStandP2(void)
{
    coopLeaveCombatP2();

    *(int*)&ply2.mode0 = 1;

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
}

static void coopApplyWeapon(void)
{
    ply2.wpnr_no = coop_wpnr_no;

    *(int*)ply2.exp0 = (coop_wpnr_no < 10) ? 0 : 1;

    if (coop_wpnr_no > 1)
    {
        ply2.flg |= 0x20000;
    }
    else
    {
        ply2.flg &= ~0x20000;
    }

    ply2.mlwP->owP[7].flg &= ~0x2;
    ply2.mlwP->owP[8].flg &= ~0x2;
    ply2.mlwP->owP[9].flg &= ~0x2;
    ply2.mlwP->owP[11].flg &= ~0x2;
    ply2.mlwP->owP[12].flg &= ~0x2;
    ply2.mlwP->owP[13].flg &= ~0x2;
}

/* G4: al final de bhReadWeaponData. Clona los objetos de arma de P1 (manos) para P2. */
void coopCloneWeapon(void)
{
    int k;
    int i;
    int n;
    int used;
    O_WRK* src;
    O_WRK* dst;
    NJS_CNK_OBJECT* so;
    NJS_CNK_OBJECT* dobj;
    O_WORK* ow;

    coop_wpn_ok[0] = 0;
    coop_wpn_ok[1] = 0;

    if (coop_enabled == 0)
    {
        return;
    }

    used = 0;

    for (k = 0; k < 2; k++)
    {
        src = &sys->obwp[k];
        dst = &coop_wpn[k];

        if ((!(src->flg & 0x1)) || (src->mlwP == NULL) || (src->mlwP->objP == NULL) || (src->mlwP->owP == NULL))
        {
            continue;
        }

        n = src->mlwP->obj_num;

        if ((used + ALIGN_UP(n * (int)sizeof(NJS_CNK_OBJECT), 64) + ALIGN_UP(n * (int)sizeof(O_WORK), 64)) > coop_wpn_area_size)
        {
            printf("[COOP] las manos de P2 no caben\n");
            continue;
        }

        npCopyMemory((unsigned char*)dst, (unsigned char*)src, sizeof(O_WRK));

        dst->mlwP = &dst->mdl[src->mlwP - src->mdl];
        dst->mtx = (void*)dst->mtxbuf;

        so = src->mlwP->objP;
        dobj = (NJS_CNK_OBJECT*)&coop_wpn_area[used];

        npCopyMemory((unsigned char*)dobj, (unsigned char*)so, n * sizeof(NJS_CNK_OBJECT));

        for (i = 0; i < n; i++)
        {
            if ((dobj[i].child != NULL) && (dobj[i].child >= so) && (dobj[i].child < &so[n]))
            {
                dobj[i].child = &dobj[dobj[i].child - so];
            }

            if ((dobj[i].sibling != NULL) && (dobj[i].sibling >= so) && (dobj[i].sibling < &so[n]))
            {
                dobj[i].sibling = &dobj[dobj[i].sibling - so];
            }
        }

        used += ALIGN_UP(n * (int)sizeof(NJS_CNK_OBJECT), 64);

        ow = (O_WORK*)&coop_wpn_area[used];

        npCopyMemory((unsigned char*)ow, (unsigned char*)src->mlwP->owP, n * sizeof(O_WORK));

        used += ALIGN_UP(n * (int)sizeof(O_WORK), 64);

        dst->mlwP->objP = dobj;
        dst->mlwP->owP = ow;
        dst->lkwkp = (unsigned char*)&ply2;

        coop_wpn_ok[k] = 1;
    }

    /* El mechero (1) es de P1: P2 lo lleva en la mano, pero sin su luz. */
    coop_wpnr_no = (ply.wpnr_no == 1) ? 0 : ply.wpnr_no;

    if ((coop_loaded != 0) && (ply2.mlwP != NULL) && (ply2.exp0 != NULL))
    {
        if (ply2.mode1 == 1)
        {
            coopStandP2();
        }

        coopApplyWeapon();
    }
}

/* Coleta de P2: como bhSetObject(lkmtab, 2, ...) y bhSetPlayer (player.c, case 0), pero en un O_WRK propio. */
static void coopSetHair(void)
{
    O_WRK* op;
    ETTY_WORK* otp;

    op = &coop_hair;
    otp = &lkmtab[0];

    coop_hair_ok = 0;

    npSetMemory((unsigned char*)op, sizeof(O_WRK), 0);

    if ((coop_mdl_n <= otp->mdlver) || (coop_mdl[otp->mdlver].objP == NULL))
    {
        return;
    }

    op->flg = otp->flg;
    op->id = otp->id;
    op->type = (unsigned char)otp->type;
    op->param = otp->type >> 8;
    op->flr_no = otp->flr_no;
    op->mdlver = otp->mdlver;
    op->draw_tp = otp->prm1;
    op->aspd = otp->aspd;

    op->sx = op->sxb = 1.0f;
    op->sy = op->syb = 1.0f;
    op->sz = op->szb = 1.0f;

    op->hide[0] = otp->hide[0];
    op->hide[1] = otp->hide[1];
    op->hide[2] = otp->hide[2];
    op->hide[3] = otp->hide[3];

    op->lkwkp = (unsigned char*)&ply2;
    op->mtx = (void*)op->mtxbuf;

    op->clp_jno[0] = 0;
    op->clp_jno[1] = -1;

    op->lkono = 5;

    op->lox = 0;
    op->loy = 1.5869f;
    op->loz = 0.7747f;

    op->skp[0] = coop_skp[op->mdlver];
    op->mdl[0] = coop_mdl[op->mdlver];
    op->mlwP = &op->mdl[0];

    /* Buffers propios: si no, bhObjClpn con plp == &ply2 usaría sys->pletcp, el pelo de P1. */
    npSetMemory(coop_hair_exp0, COOP_HAIR_EXP0_SIZE, 0);
    npSetMemory(coop_hair_exp3, COOP_HAIR_EXP3_SIZE, 0);

    op->exp0 = coop_hair_exp0;
    op->exp3 = coop_hair_exp3;
    op->flg |= 0x100000;

    op->mode0 = 0;

    coop_hair_ok = 1;
}

void coopRoomStart(void)
{
    int i;

    coop_hidden = 1;

    if ((coop_loaded == 0) || (coopDemo() != 0))
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
        ply2.skp[i] = coop_skp[i];
        ply2.mbp[i] = coop_mbp[i];
        ply2.txp[i] = coop_txp[i];
    }

    ply2.mdl_n = coop_mdl_n;
    ply2.mdl_no = 0;
    ply2.mlwP = &ply2.mdl[0];
    ply2.mlwP->texP = ply2.txp[0];
    ply2.mtx = (float(*)[16])ply2.mtxbuf;

    ply2.exp0 = coop_exp0;
    ply2.exp1 = coop_exp1;
    ply2.exp3 = NULL;

    ply2.flg = 0x119;
    ply2.mdflg = 0x20;
    ply2.stflg = 0x40000000 | 0x1000000;

    ply2.ar = PlyInfo[0][0];
    ply2.ah = PlyInfo[0][1];
    ply2.car = PlyInfo[0][0] - 1.0f;
    ply2.cah = PlyInfo[0][1] - 1.0f;

    ply2.sx = ply2.sxb = 1.0f;
    ply2.sy = ply2.syb = 1.0f;
    ply2.sz = ply2.szb = 1.0f;

    ply2.hp = COOP_P2_HP;
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

    coopApplyWeapon();

    for (i = 0; i < 2; i++)
    {
        if (coop_wpn_ok[i] != 0)
        {
            coop_wpn[i].lkwkp = (unsigned char*)&ply2;
            coop_wpn[i].stflg &= ~0x1000000;
        }
    }

    coopSetHair();

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

/* Lo que hace bhControlObjItm (objitm.c) con un objeto enganchado, sobre los objetos de P2. */
static void coopControlObject(O_WRK* op, int hair)
{
    BH_PWORK* pp;

    pp = &ply2;

    if ((pp->stflg & 0x1000000))
    {
        op->stflg |= 0x1000000;
        return;
    }

    op->stflg &= ~0x1000000;

    if (!(op->flg & 0x1))
    {
        return;
    }

    op->pxb = op->px;
    op->pyb = op->py;
    op->pzb = op->pz;
    op->axb = op->ax;
    op->ayb = op->ay;
    op->azb = op->az;

    njCalcPoint(&pp->mlwP->owP[op->lkono].mtx, (NJS_POINT3*)&op->lox, (NJS_POINT3*)&op->px);

    if ((op->flg & 0xC80000))
    {
        bhActionWeapon((BH_PWORK*)op);
    }

    if (hair != 0)
    {
        bhObjClpn(op);
    }
    else
    {
        bhObjWpn((BH_PWORK*)op);
    }

    bhCalcModel((BH_PWORK*)op);
}

static void coopDrawObject(O_WRK* op, int ok)
{
    if ((ok == 0) || (op->mdflg & 0x1) || (op->mlwP == NULL) || (op->mlwP->objP == NULL))
    {
        return;
    }

    bhDrawObject(op);
}

void coopControlPlayer2(void)
{
    if ((coop_loaded == 0) || (coopDemo() != 0))
    {
        return;
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

    /* Solo durante bhControlPlayer: los objetos de P2 se actualizan después con su propio mlwP. */
    coopSwapWeaponObj();
    bhControlPlayer();
    coopSwapWeaponObj();

    ply2.psh_ct = 0;
    ply2.stflg &= ~0x80;

    coopSeparate();

    ply2.psh_ct = 0;
    ply2.stflg &= ~0x80;

    if ((sys->sp_flg & 0x4))
    {
        if (coop_hair_ok != 0)
        {
            coopControlObject(&coop_hair, 1);
        }

        if (coop_wpn_ok[0] != 0)
        {
            coopControlObject(&coop_wpn[0], 0);
        }

        if (coop_wpn_ok[1] != 0)
        {
            coopControlObject(&coop_wpn[1], 0);
        }
    }

    coopEnd();
}

void coopDrawPlayer2(void)
{
    if ((coop_loaded == 0) || (coopDemo() != 0))
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

    coopDrawObject(&coop_hair, coop_hair_ok);
    coopDrawObject(&coop_wpn[0], coop_wpn_ok[0]);
    coopDrawObject(&coop_wpn[1], coop_wpn_ok[1]);
}

#endif
