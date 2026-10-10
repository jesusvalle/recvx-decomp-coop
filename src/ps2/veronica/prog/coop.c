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
#include "../../../ps2/veronica/prog/ps2_NaMatrix.h"
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
#include "../../../ps2/veronica/prog/sub1.h"
#include "../../../ps2/veronica/prog/item.h"
#include "../../../ps2/veronica/prog/message.h"

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
static O_WRK coop_wpn[2];
static int coop_wpn_ok[2];
static MN_WORK* coop_mnw2;
static unsigned char* coop_wmt2;
static unsigned char* coop_wmdl2;
static int coop_wpn2_tex[2];
static int coop_wpn2_no;
static int coop_wpn2_req;
static int coop_mounted;
static COOP_PAD coop_pad2raw;
static COOP_PAD coop_pad1;
static int coop_inv_owner;
static int coop_inj;
static unsigned int coop_inj_ps;
static unsigned int coop_inj_on;
static unsigned int coop_p2_req;
static int coop_req_sb;
static int coop_req_etc;
static int coop_p1_sb;
static int coop_p1_etc;
static unsigned int coop_p1_cb100;
static unsigned int coop_gm_crit;
static unsigned int coop_mn_pre;
static unsigned int coop_pend;
static int coop_lid = -1;
static int coop_req_ct;
static int coop_unstick;
static unsigned char coop_tgt[128];
static int coop_tgt_frm[128];
static int coop_etgt;
static int coop_eport;
static int coop_efx;
static int coop_efport;
static O_WRK coop_hair;
static unsigned char coop_ene4[128];
static LGT_WORK coop_lgt0;
static int coop_port_bak;
static int coop_hair_ok;
static unsigned int coop_gm2;
static unsigned int* coop_pip_bak;

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
    float fade[4];
} COOP_SAVE;

static COOP_SAVE coop_save;
static int coop_hidden;
static int coop_shadow = -1;

static int coopReadWeapon2Data(unsigned char* datp);
static int coopP2Dead(void);

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

static void coopGetPad(COOP_PAD* p)
{
    p->on = sys->pad_on;
    p->oncpy = sys->pad_oncpy;
    p->ps = sys->pad_ps;
    p->rs = sys->pad_rs;
    p->old = sys->pad_old;
    p->onb = sys->pad_onb;
    p->psb = sys->pad_psb;
    p->oldb = sys->pad_oldb;
    p->ax = sys->pad_ax;
    p->ay = sys->pad_ay;
    p->dx = sys->pad_dx;
    p->dy = sys->pad_dy;
    p->ar = sys->pad_ar;
    p->al = sys->pad_al;
}

static void coopPutPad(const COOP_PAD* p)
{
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
}

void coopSetPad2(void)
{
    int port_bak;
    const PDS_PERIPHERAL* per_bak;
    unsigned int mask;

    if (((sys->ss_flg & 0xC00000)) || (!(sys->sp_flg & 0x20)))
    {
        coop_pad2.on = coop_pad2.oncpy = coop_pad2.ps = coop_pad2.rs = coop_pad2.old = 0;
        coop_pad2raw.on = coop_pad2raw.oncpy = coop_pad2raw.ps = coop_pad2raw.rs = coop_pad2raw.old = 0;

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

    coop_pad2raw = coop_pad2;

    mask = COOP_P2_PAD_MASK;

    /* Con armas de mira, P2 no apunta: la mira cambia la cámara y el dibujo para todos. */
    if ((WpnTab[ply2.wpnr_no].flg & 0x20))
    {
        mask &= ~0x10;
    }

    /* oncpy no se enmascara: es el historial con el que bhSetPad calcula ps, y los botones de fuera de la máscara (Start, acción, menús) tienen que dar una sola pulsación. */
    coop_pad2.on &= mask;
    coop_pad2.ps &= mask;
    coop_pad2.rs &= mask;
    coop_pad2.old &= mask;

    /* Con la pantalla de P2 abierta, las tareas 8 y 9 leen su mando, sin máscara. */
    if (coop_inv_owner == 2)
    {
        coopPutPad(&coop_pad2raw);
    }

    /* P2 muriendo: P1 se queda quieto hasta el game over (bhCPM0_die bloquea al jugador que muere; aquí, una puerta resucitaría a P2). */
    if ((coop_mounted != 0) && (coopP2Dead() != 0) && (sys->ts_flg & 0x4000))
    {
        sys->pad_on = 0;
        sys->pad_ps = 0;
        sys->pad_rs = 0;
        sys->pad_old = 0;
        sys->pad_ax = 0;
        sys->pad_ay = 0;
        sys->pad_dx = 0;
        sys->pad_dy = 0;
    }
}

void coopInitMemory(void)
{
    coop_enabled = 0;
    coop_loaded = 0;
    coop_ld_mode = 0;
    coop_mdl_n = 0;

    /* Tras njReleaseTextureAll los texlists viejos no valen: no se liberan. */
    coop_mounted = 0;
    coop_inv_owner = 0;
    coop_p2_req = 0;
    coop_wpn2_no = 0;
    coop_gm2 = 0;
    coop_wpn_ok[0] = coop_wpn_ok[1] = 0;
    coop_wpn2_tex[0] = coop_wpn2_tex[1] = 0;

    coop_exp0 = bhGetFreeMemory(sizeof(EXP_WORK), 32);
    coop_exp1 = bhGetFreeMemory(124, 32);
    coop_pool = bhGetFreeMemory(COOP_MODEL_POOL_SIZE, 64);
    coop_mnw2 = (MN_WORK*)bhGetFreeMemory(COOP_MNW_N * sizeof(MN_WORK), 32);
    coop_wmt2 = bhGetFreeMemory(COOP_WMT_SIZE, 64);
    coop_wmdl2 = bhGetFreeMemory(COOP_WMDL_SIZE, 64);

    if ((coop_exp0 == NULL) || (coop_exp1 == NULL) || (coop_pool == NULL) || (coop_mnw2 == NULL) || (coop_wmt2 == NULL) || (coop_wmdl2 == NULL) || (sys->lmmdlp == NULL) || (sys->memp > sys->endp))
    {
        coop_pool = NULL;
        coop_mnw2 = NULL;

        printf("[COOP] sin memoria para P2: desactivado\n");
        return;
    }

    /* sys->lmmdlp (32 KB) no lo usa el juego: buffers de la coleta de P2 */
    coop_hair_exp3 = sys->lmmdlp;
    coop_hair_exp0 = &sys->lmmdlp[COOP_HAIR_EXP3_SIZE];

    npSetMemory((unsigned char*)coop_mnw2, COOP_MNW_N * sizeof(MN_WORK), 0);

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

/* Id de objeto → arma, como WeaponSet (sub1.c). El mechero (55) es de P1: P2 va sin arma. */
static int coopItemToWpn(unsigned int e)
{
    unsigned int id;

    id = e >> 16;

    switch ((unsigned char)id)
    {
    case 1:
        return 20;
    case 2:
        return 12;
    case 3:
        return 13;
    case 4:
        return 11;
    case 5:
        return 3;
    case 6:
        switch (id & 0x7000)
        {
        case 0x1000:
            return 15;
        case 0x2000:
            return 16;
        case 0x4000:
            return 17;
        }

        return 14;
    case 7:
        if ((id & 0x2000))
        {
            return 19;
        }

        return 10;
    case 8:
        return 2;
    case 9:
        return 4;
    case 10:
        return 5;
    case 11:
        return 18;
    case 32:
        return 6;
    case 33:
        return 7;
    case 34:
        return 8;
    case 131:
        return 3;
    case 142:
        return 9;
    }

    return 0;
}

/* Entrada equipada del bloque de P2 (0 si no hay o si la casilla no es válida). */
static unsigned int coopEquipped2(void)
{
    unsigned int* pip;

    pip = &sys->itm[COOP_ITM];

    if ((pip[0] == 0) || (pip[0] >= 12))
    {
        return 0;
    }

    return pip[pip[0]];
}

/* Bloque de inventario de P2 (sys->itm[256..279]): se guarda con la partida. Si no está, se crea con el cuchillo. */
static void coopSeedBlock(void)
{
    unsigned int* pip;
    int i;

    pip = &sys->itm[COOP_ITM];

    if (pip[16] == COOP_MAGIC)
    {
        return;
    }

    for (i = 0; i < 24; i++)
    {
        pip[i] = 0;
    }

    pip[0] = 2;
    pip[2] = 0x00080001;

    pip[16] = COOP_MAGIC;
    pip[17] = (sys->gm_mode == 2) ? 320 : 160;
    pip[18] = 0;

    printf("[COOP] bloque de P2 creado (cuchillo)\n");
}

#ifdef COOP_TEST
/* Solo para pruebas: pistola (id 5, 15 balas) equipada en el bloque de P2. */
static void coopTestGiveHandgun(void)
{
    unsigned int* pip;
    int i;

    pip = &sys->itm[COOP_ITM];

    for (i = 2; i < 10; i++)
    {
        if (((pip[i] >> 16) & 0xFF) == 5)
        {
            pip[0] = i;
            return;
        }
    }

    for (i = 2; i < 10; i++)
    {
        if (pip[i] == 0)
        {
            pip[i] = 0x0005000F;
            pip[0] = i;
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

        coopSeedBlock();

#ifdef COOP_TEST
        coopTestGiveHandgun();
#endif

        coop_wpn2_req = coopItemToWpn(coopEquipped2());

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
        }
        else
        {
            printf("[COOP] error al leer el fichero %d de P2\n", coop_ld_file);
        }

        if (coop_loaded == 0)
        {
            coop_ld_mode = 3;
            return 1;
        }

        coop_ld_mode = 4;
        return 0;
    case 4:
        if (GetReadFileStatus() != 0)
        {
            return 0;
        }

        coop_ld_file = coop_wpn2_req + 20;

        size = GetInsideFileSize(sys->sys_partid, coop_ld_file);

        coop_ld_buf = (unsigned char*)ALIGN_UP((int)sys->memp, 64);

        if ((size <= 0) || (&coop_ld_buf[size] > (sys->endp - COOP_PXLCONV_SIZE)))
        {
            printf("[COOP] no se puede leer el arma %d de P2 (%d B)\n", coop_ld_file, size);

            coop_wpn2_no = 0;

            coop_ld_mode = 3;
            return 1;
        }

        if (RequestReadInsideFile(sys->sys_partid, coop_ld_file, coop_ld_buf) != 0)
        {
            return 0;
        }

        coop_ld_mode = 5;
        return 0;
    case 5:
        st = GetReadFileStatus();

        if (st == 1)
        {
            return 0;
        }

        if (st == 0)
        {
            coopReadWeapon2Data(coop_ld_buf);
        }
        else
        {
            printf("[COOP] error al leer el arma %d de P2\n", coop_ld_file);

            coop_wpn2_no = 0;
        }

        coopSeLoad(WpnTab[coop_wpn2_no].snd_wpno);

        coop_ld_mode = 6;
        return 0;
    case 6:
        if (coopSeStep() == 0)
        {
            return 0;
        }

        coop_ld_mode = 3;
        return 1;
    }

    return 1;
}

/* G11: modo 3 del cargador marcado como de P2 (mn_md3 == COOP_MN_P2). Usa mn_md1 como paso; devuelve 1 al terminar. */
int coopMonitorWeapon2(void)
{
    int file;
    int size;
    int st;

    if ((coop_enabled == 0) || (coop_loaded == 0))
    {
        return 1;
    }

    switch (sys->mn_md1)
    {
    case 0:
        if (GetReadFileStatus() == 1)
        {
            return 0;
        }

        coop_wpn2_req = coopItemToWpn(coopEquipped2());

        file = coop_wpn2_req + 20;

        size = GetInsideFileSize(sys->sys_partid, file);

        sys->memp = (unsigned char*)ALIGN_UP((int)sys->memp, 64);

        if ((size <= 0) || (&sys->memp[size] > (sys->endp - COOP_PXLCONV_SIZE)))
        {
            printf("[COOP] no se puede leer el arma %d de P2 (%d B)\n", file, size);
            return 1;
        }

        if (RequestReadInsideFile(sys->sys_partid, file, sys->memp) != 0)
        {
            return 0;
        }

        sys->mn_md1 = 1;
        return 0;
    case 1:
        st = GetReadFileStatus();

        if (st == 1)
        {
            return 0;
        }

        if (st == 0)
        {
            coopReadWeapon2Data(sys->memp);
        }
        else
        {
            printf("[COOP] error al leer el arma de P2\n");
        }

        coopSeLoad(WpnTab[coop_wpn2_no].snd_wpno);

        sys->mn_md1 = 2;
        return 0;
    case 2:
        return coopSeStep();
    }

    return 1;
}

/* G13, antes de bhCheckSubTask: el Start o la petición de P2 se inyectan como si fueran de P1. */
void coopPreSubTask(void)
{
    coop_inj = 0;

    if ((coop_loaded == 0) || (coop_mounted == 0) || (coopDemo() != 0) || (coop_inv_owner != 0) || (coop_hidden != 0))
    {
        return;
    }

    /* La tapa del baúl que abrió P2 pide la pantalla (cb_flg 0x40000) desde bhObjItmBox: es de P2. */
    if (coop_lid >= 0)
    {
        if ((ply.stflg & 0x8000))
        {
            /* P1 también está en la tapa (bhCheckExmAtari la reinició): el baúl es de P1 y P2 se levanta. */
            coop_lid = -1;
            coop_unstick = 1;
        }
        else if ((sys->cb_flg & 0x40000) && (sys->obwp[coop_lid].mode0 == 3))
        {
            sys->cb_flg &= ~0x40000;

            coop_p2_req = 0x40000;
            coop_lid = -1;
        }
        else if (sys->obwp[coop_lid].type != 100)
        {
            /* La tapa se interrumpió: P2 no puede quedarse en la acción forzada (stflg 0x8000). */
            coop_lid = -1;
            coop_unstick = 1;
        }
    }

    if ((sys->ss_flg & 0x80000000) || (sys->st_flg & 0x1C040008) || (sys->cb_flg & 0x2064017) || (!(sys->ts_flg & 0x1000)))
    {
        return;
    }

    /* P1 ocupado (escalera, examinar, agacharse...) o con un mensaje: la petición de P2 espera. */
    if ((sys->st_flg & 0x4) || (sys->st_flg & 0x200))
    {
        return;
    }

    if ((ply2.stflg & 0x1000000) || (ply2.flg & 0x6) || (ply2.hp < 0))
    {
        return;
    }

    if (coop_p2_req != 0)
    {
        coop_p1_sb = sys->sb_id;
        coop_p1_etc = sys->etc_idx;
        coop_p1_cb100 = sys->cb_flg & 0x100;

        sys->sb_id = coop_req_sb;
        sys->etc_idx = coop_req_etc;
        sys->cb_flg |= 0x100 | coop_p2_req;

        coop_inj = 2;
    }
    else if ((coop_pad2raw.ps & 0x4000) && (!(coop_pad2raw.on & 0x80)) && (!(sys->pad_ps & 0x4000)))
    {
        coop_inj_ps = sys->pad_ps;
        coop_inj_on = sys->pad_on;

        sys->pad_ps |= 0x4000;
        sys->pad_on &= ~0x80;

        coop_inj = 1;
    }
}

/* G13, después de bhCheckSubTask: si se abrió el inventario, es de P2; si no, se deshace la inyección. */
void coopPostSubTask(void)
{
    if (coop_inj == 0)
    {
        return;
    }

    if (coop_inj == 1)
    {
        sys->pad_ps = coop_inj_ps;
        sys->pad_on = coop_inj_on;
    }

    if ((sys->st_flg & 0x8))
    {
        if (coop_inj == 1)
        {
            coop_p1_sb = sys->sb_id;
            coop_p1_etc = sys->etc_idx;
            coop_p1_cb100 = sys->cb_flg & 0x100;
        }

        coop_inv_owner = 2;
        coop_p2_req = 0;

        /* bhSetPad calcula las pulsaciones con el historial de sys->pad_*: al cerrar se repone el de P1. */
        coopGetPad(&coop_pad1);
        coop_pad1.ps = 0;

        coopVibStop2();

        printf("[COOP] pantalla de P2 abierta\n");
    }
    else if (coop_inj == 2)
    {
        sys->cb_flg = (sys->cb_flg & ~(coop_p2_req | 0x100)) | coop_p1_cb100;
        sys->sb_id = coop_p1_sb;
        sys->etc_idx = coop_p1_etc;
    }

    coop_inj = 0;
}

/* G14 (StatusMain, inicialización, antes de CursorInit): la pantalla de P2 usa su bloque. */
void coopStatusInit(void)
{
    S_WORK* st;

    if (coop_inv_owner != 2)
    {
        return;
    }

    st = &swork;

    st->pip = &sys->itm[COOP_ITM];

    if ((st->subscreenmode == 0x2) || (st->subscreenmode == 0x4))
    {
        st->itemid = (st->pip[st->listcsr_0] >> 16) & 0xFF;
    }
}

/* G15 (bhSysCallItemselect): con la pantalla de P2, plp = &ply2 alrededor de ItemTaskCheck y StatusMain. */
void coopItemselectBegin(void)
{
    if (coop_inv_owner != 2)
    {
        return;
    }

    coop_mn_pre = *(int*)&sys->mn_mode0;
    coop_gm_crit = sys->gm_flg & 0x10000000;

    plp = &ply2;
}

void coopItemselectEnd(void)
{
    if (coop_inv_owner != 2)
    {
        return;
    }

    plp = &ply;

    /* WeaponSet escribe el crítico en gm_flg: el de P2 se calcula al cargar su arma (2c). */
    sys->gm_flg = (sys->gm_flg & ~0x10000000) | coop_gm_crit;

    /* Equipar (WeaponSet, GetItem, ItemBoxChange) pide el modo 3: es el cargador de P2. */
    if ((sys->mn_mode0 == 3) && ((coop_mn_pre & 0xFF) != 3))
    {
        sys->mn_mode3 = COOP_MN_P2;
    }

    /* Fin real de la sesión (el paso por el mapa no la cierra). */
    if ((sys->ts_flg & 0x200) && (!(sys->st_flg & 0x40000)))
    {
        swork.pip = &sys->itm[sys->ply_id * 16];

        sys->sb_id = coop_p1_sb;
        sys->etc_idx = coop_p1_etc;
        sys->cb_flg = (sys->cb_flg & ~0x100) | coop_p1_cb100;

        coopPutPad(&coop_pad1);

        /* Curarse en su pantalla cambia la vida de P2. */
        sys->itm[COOP_ITM + 17] = (unsigned int)ply2.hp;
        sys->itm[COOP_ITM + 18] = ply2.stflg & 0x280000;

        coop_inv_owner = 0;

        printf("[COOP] pantalla de P2 cerrada\n");
    }
}

/* G16 (ItemUse): en la pantalla de P2, lo que va a un activador de suelo (Use_01/Use_05) no se usa: el activador es el de P1. */
int coopItemUseBlocked(S_WORK* st)
{
    unsigned int t;

    if (coop_inv_owner != 2)
    {
        return 0;
    }

    t = itemdata[(unsigned char)(st->pip[st->listcsr_0] >> 16)].type & 0x5F;

    if ((t != 0x2) && (t != 0x40) && (t != 0x1))
    {
        return 0;
    }

    if (!(sys->st_flg & 0x200))
    {
        bhSetMessage(1, 160);

        swork.statusflg &= ~0x100000;
    }

    return 1;
}

/* G12: bhEff007 oculta el cargador del arma 12/13 del tirador; si el efecto es de P2, en el arma de P2. */
void coopEff007Mag(O_WRK* op)
{
    O_WRK* wp;

    wp = sys->obwp;

    if ((op->lkwkp == (unsigned char*)&ply2) && (coop_wpn_ok[0] != 0))
    {
        wp = &coop_wpn[0];
    }

    if ((wp->mlwP == NULL) || (wp->mlwP->objP == NULL) || (wp->mlwP->obj_num <= 2))
    {
        return;
    }

    wp->mlwP->objP[2].evalflags |= 0x8;
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
    coop_save.fade[0] = sys->fade_an;
    coop_save.fade[1] = sys->fade_rn;
    coop_save.fade[2] = sys->fade_gn;
    coop_save.fade[3] = sys->fade_bn;

    coop_pip_bak = swork.pip;
    swork.pip = &sys->itm[COOP_ITM];

    sys->gm_flg = (sys->gm_flg & ~0x10040000) | coop_gm2;

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
    /* Arma vacía y crítico de la pistola especial: cada jugador tiene los suyos. */
    coop_gm2 = sys->gm_flg & 0x10040000;
    sys->gm_flg = coop_save.gm_flg;
    sys->pt_flg = coop_save.pt_flg;
    sys->flr_idx = coop_save.flr_idx;
    sys->etc_idx = coop_save.etc_idx;
    sys->pl_htp = coop_save.pl_htp;
    sys->door = coop_save.door;
    cam = coop_save.cam;

    /* bhCPM0_die corta el fundido en curso: el de P1 no se toca. */
    sys->fade_an = coop_save.fade[0];
    sys->fade_rn = coop_save.fade[1];
    sys->fade_gn = coop_save.fade[2];
    sys->fade_bn = coop_save.fade[3];

    CurrentPortId = coop_port_bak;

    swork.pip = coop_pip_bak;

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

/* P2 muerto o muriendo: no se le reanima ni se le congela antes de que su animación lance el game over. */
static int coopP2Dead(void)
{
    if ((ply2.hp < 0) || (ply2.mode0 == 3) || ((ply2.mode0 == 6) && (ply2.flg & 0x2)))
    {
        return 1;
    }

    return 0;
}

/* Como bhResetPlayer (player.c): animaciones de cojera según la vida y el veneno. */
static void coopSetDmlvl(void)
{
    EXP_WORK* e;

    e = (EXP_WORK*)ply2.exp0;

    if (ply2.hp >= 120)
    {
        e->dmlvl = ((ply2.stflg & 0x280000)) ? 1 : 0;
    }
    else if (ply2.hp >= 30)
    {
        e->dmlvl = 1;
    }
    else
    {
        e->dmlvl = 2;
    }
}

/* P2 puede ser objetivo de enemigos y efectos: cargado, visible, fuera de puertas y vivo. */
static int coopP2Target(void)
{
    if ((coop_loaded == 0) || (coop_mounted == 0) || (coopDemo() != 0) || (coop_hidden != 0))
    {
        return 0;
    }

    if ((ply2.stflg & 0x81000000) || (coopP2Dead() != 0))
    {
        return 0;
    }

    return 1;
}

/* Enemigos que eligen al jugador más cercano (D14). Jefes, trampas, grúa, Spotter y polilla: siempre P1. */
static int coopEnemyChooses(int id)
{
    switch (id)
    {
    case 1:
    case 3:
    case 4:
    case 5:
    case 7:
    case 9:
    case 10:
    case 21:
    case 22:
    case 23:
    case 24:
    case 26:
    case 30:
        return 1;
    }

    return 0;
}

/* El jugador está siendo golpeado o agarrado: el enemigo no cambia de objetivo. */
static int coopTargetBusy(BH_PWORK* pp, BH_PWORK* ep)
{
    float dx;
    float dz;

    if ((pp->mode0 == 2) || (pp->mode0 == 4) || (pp->mode0 == 5) || (pp->mode0 == 6))
    {
        return 1;
    }

    dx = pp->px - ep->px;
    dz = pp->pz - ep->pz;

    if ((pp->flg & 0x4) && (((dx * dx) + (dz * dz)) < 400.0f))
    {
        return 1;
    }

    return 0;
}

static float coopDist2(BH_PWORK* pp, float x, float z)
{
    float dx;
    float dz;

    dx = pp->px - x;
    dz = pp->pz - z;

    return (dx * dx) + (dz * dz);
}

/* G17 (bhControlEnemy, antes de bhJumpEnemy): elige objetivo y, si es P2, el update del enemigo ve a P2 como plp. */
void coopEnemyBegin(BH_PWORK* ep)
{
    BH_PWORK* lk;
    BH_PWORK* cur;
    BH_PWORK* oth;
    float dc;
    float d2;
    int idx;
    int t;
    int ok2;
    int sw;

    coop_etgt = 0;

    idx = ep - ene;

    if ((idx < 0) || (idx >= 128))
    {
        return;
    }

    if ((coop_loaded == 0) || (coop_mounted == 0) || (coopDemo() != 0))
    {
        coop_tgt[idx] = 0;
        return;
    }

    /* P2 vale como objetivo nuevo solo vivo y visible; el que ya tiene un enemigo ocupado se conserva (agarres mortales). */
    ok2 = coopP2Target();

    if ((ep->flg & 0x80))
    {
        /* Partes enganchadas: las del jugador, su jugador; las de otro enemigo, el objetivo de ese enemigo. */
        lk = (BH_PWORK*)ep->lkwkp;

        if (lk == &ply2)
        {
            t = 1;
        }
        else if ((lk >= ene) && (lk < &ene[128]))
        {
            t = coop_tgt[lk - ene];
        }
        else
        {
            t = 0;
        }

        coop_tgt[idx] = t;
    }
    else if (coopEnemyChooses(ep->id) != 0)
    {
        t = coop_tgt[idx];

        cur = (t != 0) ? &ply2 : &ply;
        oth = (t != 0) ? &ply : &ply2;

        /* Bloqueo: el enemigo no está en su estado normal o su objetivo está siendo golpeado o agarrado. */
        if ((ep->mode0 == 1) && (coopTargetBusy(cur, ep) == 0))
        {
            if ((t != 0) && (ok2 == 0))
            {
                /* P2 ya no vale (muerto, oculto o en una puerta) y el enemigo está libre: vuelve a P1. */
                coop_tgt[idx] = 0;
                coop_tgt_frm[idx] = sys->gfrm_ct;
            }
            else if ((ok2 != 0) && ((unsigned int)(sys->gfrm_ct - coop_tgt_frm[idx]) >= 30))
            {
                dc = coopDist2(cur, ep->px, ep->pz);
                d2 = coopDist2(oth, ep->px, ep->pz);

                sw = 0;

                if ((oth->flr_no == ep->flr_no) && (cur->flr_no != ep->flr_no))
                {
                    sw = 1;
                }
                else if (((oth->flr_no == ep->flr_no) || (cur->flr_no != ep->flr_no)) && (d2 < (0.5625f * dc)))
                {
                    /* 0,75 de la distancia (0,5625 del cuadrado). */
                    sw = 1;
                }

                if (sw != 0)
                {
                    coop_tgt[idx] = (t != 0) ? 0 : 1;
                    coop_tgt_frm[idx] = sys->gfrm_ct;
                }
            }
        }
    }
    else
    {
        coop_tgt[idx] = 0;
    }

    if (coop_tgt[idx] != 0)
    {
        coop_etgt = 1;

        coop_eport = CurrentPortId;
        CurrentPortId = 1;

        coopSwapPad(&coop_pad2);

        plp = &ply2;
    }
}

/* G17 (después de bhJumpEnemy): deshace el cambio en orden inverso. */
void coopEnemyEnd(BH_PWORK* ep)
{
    if (coop_etgt == 0)
    {
        return;
    }

    plp = &ply;

    coopSwapPad(&coop_pad2);

    CurrentPortId = coop_eport;

    coop_etgt = 0;
}

/* G18 (bhControlEffect): los efectos que dañan al jugador alcanzan al más cercano. Solo cambian plp y el puerto. */
void coopEffectBegin(O_WRK* op)
{
    coop_efx = 0;

    switch (op->id)
    {
    case 256:
    case 260:
    case 265:
    case 266:
    case 269:
    case 350:
    case 397:
        break;
    default:
        return;
    }

    if (coopP2Target() == 0)
    {
        return;
    }

    if (coopDist2(&ply2, op->px, op->pz) < coopDist2(&ply, op->px, op->pz))
    {
        coop_efx = 1;

        coop_efport = CurrentPortId;
        CurrentPortId = 1;

        plp = &ply2;
    }
}

void coopEffectEnd(O_WRK* op)
{
    if (coop_efx != 0)
    {
        plp = &ply;

        CurrentPortId = coop_efport;

        coop_efx = 0;
    }

    /* Gas de sala (bhEff127, case 1): cada llamada sube el gas, así que no se repite: se compara la cabeza de P2. */
    if ((op->id == 127) && (op->type == 1) && (op->mode0 == 1) && (coopP2Target() != 0) && (ply2.mlwP != NULL) && (ply2.mlwP->owP != NULL))
    {
        if (((1.0f + ply2.mlwP->owP[5].mtx[13]) < sys->gas_py) && (!(ply2.stflg & 0x40000)))
        {
            ply2.hp = -1;
        }
    }
}

/* G19 (principio de bhCheckBombAtari): el bloque del jugador, repetido sobre P2 (sin st_flg 0x4, que es de P1). */
void coopCheckBombP2(NJS_POINT3* pos, float ar, int dmax, int dmin)
{
    NJS_CAPSULE wal;
    NJS_SPHERE spr;
    NJS_VECTOR sca;
    NJS_POINT3 ps;
    NJS_VECTOR vec0;
    NJS_VECTOR vec1;
    float inn;
    BH_PWORK* pp;

    if (coopP2Target() == 0)
    {
        return;
    }

    pp = &ply2;

    if ((!(pp->flg & 0x1)) || (pp->flg & 0x2))
    {
        return;
    }

    spr.c.x = pos->x;
    spr.c.y = pos->y;
    spr.c.z = pos->z;

    spr.r = 0.7f * ar;

    if (npCollisionCheckSC(&spr, &pp->watr) == 0)
    {
        return;
    }

    npDistanceP2C(pos, &pp->watr, &ps);

    wal.c1.x = ps.x;
    wal.c1.y = ps.y;
    wal.c1.z = ps.z;

    wal.c2.x = pos->x;
    wal.c2.y = pos->y;
    wal.c2.z = pos->z;

    wal.r = 0.1f;

    if (bhCheckC2WallN(&wal, 0x400) != 0)
    {
        return;
    }

    pp->dpx = ps.x;
    pp->dpy = ps.y;
    pp->dpz = ps.z;

    pp->dvx = ps.x - pos->x;
    pp->dvy = ps.y - pos->y;
    pp->dvz = ps.z - pos->z;

    pp->dax = 0;
    pp->day = (int)(10430.381f * atan2f(-pp->dvx, -pp->dvz));

    vec0.x = -njSin(pp->ay);
    vec0.y = 0;
    vec0.z = -njCos(pp->ay);

    vec1.x = -pp->dvx;
    vec1.y = 0;
    vec1.z = -pp->dvz;

    njUnitVector(&vec1);

    inn = njInnerProduct(&vec0, &vec1);

    if (inn > 0)
    {
        *(int*)&pp->mode0 = 0x20002;
    }
    else
    {
        *(int*)&pp->mode0 = 0x20102;
    }

    pp->flg |= 0x10004;
    pp->flg2 |= 0x200;

    sca.x = pp->px - pos->x;
    sca.y = pp->py - pos->y;
    sca.z = pp->pz - pos->z;

    if (njScalor(&sca) < (0.5f * spr.r))
    {
        pp->hp -= dmax;
    }
    else
    {
        pp->hp -= dmin;
    }
}

/* P2 golpeado o agarrado: su pose la lleva el enemigo. */
static int coopP2Held(void)
{
    if ((ply2.mode0 == 2) || (ply2.mode0 == 4) || (ply2.mode0 == 5) || (ply2.mode0 == 6))
    {
        return 1;
    }

    return 0;
}

static int coopFreezeCondition(void)
{
    if ((!(sys->sp_flg & 0x1)) || ((sys->st_flg & 0x200) && (coopP2Dead() == 0)))
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

    /* A P2 muerto, golpeado o agarrado solo se le mueve: no se le reanima ni se le suelta del enemigo. */
    if ((coopP2Dead() == 0) && (coopP2Held() == 0))
    {
        coopLeaveCombatP2();

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
    }

    bhCalcModel(&ply2);
}

static void coopStandP2(void)
{
    if (coopP2Dead() != 0)
    {
        return;
    }

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
    ply2.wpnr_no = coop_wpn2_no;

    *(int*)ply2.exp0 = (coop_wpn2_no < 10) ? 0 : 1;

    if (coop_wpn2_no > 1)
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

/* G4: al final de bhReadPlayerData. El cuerpo de P2 usa las animaciones de P1 (0-99); las del arma (100+) son suyas. */
void coopSyncBodyMotions(void)
{
    if ((coop_enabled == 0) || (coop_mnw2 == NULL))
    {
        return;
    }

    npCopyMemory((unsigned char*)coop_mnw2, (unsigned char*)sys->plmthp, 100 * sizeof(MN_WORK));
}

/* Copia reducida de bhReadWeaponData (dread.c) que no toca plp, sys->obwp, wrmdlp, wlmdlp ni plwmtp. */
static int coopReadWeapon2Data(unsigned char* datp)
{
    O_WRK* op;
    MN_WORK* mtnp;
    unsigned char* dp;
    unsigned char* mp;
    unsigned char* mdp[2];
    unsigned char* tdp[2];
    unsigned int msz[2];
    unsigned int tsz[2];
    unsigned int msize;
    unsigned int need;
    int used;
    int size;
    int rel;
    int ok;
    int temp;
    int k;

    dp = datp;

    for (k = 0; k < 2; k++)
    {
        msz[k] = *(unsigned int*)dp;
        dp += 4;

        mdp[k] = dp;
        dp = &dp[msz[k]];

        temp = *(unsigned int*)dp;

        if ((temp & 0x80000000))
        {
            dp += 4;
            dp = (unsigned char*)(((int)dp + 63) & ~0x3F);
            temp &= ~0x80000000;
        }
        else
        {
            dp += 4;
        }

        tsz[k] = temp;
        tdp[k] = dp;

        dp = &dp[temp];
    }

    msize = *(unsigned int*)dp;
    dp += 4;

    ok = 1;

    if (((msz[0] + msz[1] + 512) > COOP_WMDL_SIZE) || (msize > COOP_WMT_SIZE))
    {
        printf("[COOP] el arma %d de P2 no cabe (%d + %d, %d)\n", coop_wpn2_req, msz[0], msz[1], msize);

        ok = 0;
    }

    /* Solo las texturas que cargó P2: nunca un texP de P1. */
    rel = 0;

    for (k = 0; k < 2; k++)
    {
        if ((coop_wpn2_tex[k] != 0) && (coop_wpn[k].mlwP != NULL) && (coop_wpn[k].mlwP->texP != NULL))
        {
            njReleaseTexture(coop_wpn[k].mlwP->texP);

            rel = 1;
        }

        coop_wpn2_tex[k] = 0;
        coop_wpn_ok[k] = 0;
    }

    if (rel != 0)
    {
        bhGarbageTexture(NULL, 0);
    }

    need = 0;

    for (k = 0; k < 2; k++)
    {
        if (msz[k] != 0)
        {
            need += tsz[k];
        }
    }

    if ((ok != 0) && (Ps2_free_texmemsize < need))
    {
        printf("[COOP] sin memoria de texturas para el arma de P2 (%d < %d)\n", Ps2_free_texmemsize, need);

        ok = 0;
    }

    used = 0;

    for (k = 0; (ok != 0) && (k < 2); k++)
    {
        op = &coop_wpn[k];

        npSetMemory((unsigned char*)op, sizeof(O_WRK), 0);

        if (msz[k] == 0)
        {
            continue;
        }

        mp = &coop_wmdl2[used];

        if ((used + (int)msz[k]) > COOP_WMDL_SIZE)
        {
            ok = 0;
            break;
        }

        npCopyMemory(mp, mdp[k], msz[k]);

        /* Como bhSetWeapon, pero sin efectos sobre plp. */
        op->flg = 0x81;

        if (coop_wpn2_req != 0)
        {
            op->flg |= 0x40000;
        }

        op->id = 1210;
        op->type = coop_wpn2_req & 0xFF;
        op->flr_no = ply2.flr_no;
        op->mtx = (void*)op->mtxbuf;
        op->lkono = (k == 0) ? 9 : 13;
        op->lkwkp = (unsigned char*)&ply2;
        op->mlwP = op->mdl;

        bhMlbBinRealize(mp, op->mlwP);

        used = (((int)&mp[msz[k]] + 255) & ~0xFF) - (int)coop_wmdl2;

        size = op->mlwP->obj_num * sizeof(O_WORK);

        if ((used + size) > COOP_WMDL_SIZE)
        {
            op->flg = 0;

            ok = 0;
            break;
        }

        op->mlwP->owP = (O_WORK*)&coop_wmdl2[used];

        npSetMemory((unsigned char*)op->mlwP->owP, size, 0);

        used = ALIGN_UP(used + size, 64);

        op->mdl_no = 0;

        if ((tsz[k] != 0) && (op->mlwP->texP != NULL))
        {
            op->mlwP->flg |= 0x200;

            bhSetMemPvpTexture(op->mlwP->texP, tdp[k], 0);

            coop_wpn2_tex[k] = 1;
        }

        coop_wpn_ok[k] = 1;
    }

    /* El bucle original no limpia los huecos vacíos: aquí se limpia toda la parte del arma. */
    npSetMemory((unsigned char*)&coop_mnw2[100], (COOP_MNW_N - 100) * sizeof(MN_WORK), 0);

    if (ok == 0)
    {
        printf("[COOP] el arma %d de P2 no cabe: P2 va sin arma\n", coop_wpn2_req);

        for (k = 0; k < 2; k++)
        {
            if (coop_wpn2_tex[k] != 0)
            {
                njReleaseTexture(coop_wpn[k].mlwP->texP);

                coop_wpn2_tex[k] = 0;
            }

            coop_wpn_ok[k] = 0;
            coop_wpn[k].flg = 0;
        }

        bhGarbageTexture(NULL, 0);

        coop_wpn2_no = 0;
    }
    else
    {
        if (msize != 0)
        {
            npCopyMemory(coop_wmt2, dp, msize);

            mp = coop_wmt2;

            for (mtnp = &coop_mnw2[100]; (mp < &coop_wmt2[msize]) && (mtnp < &coop_mnw2[COOP_MNW_N]) && ((temp = *(unsigned int*)mp) != -1); mtnp++)
            {
                if (temp != 0)
                {
                    bhMnbBinRealize(&mp[4], mtnp);

                    mp = &mp[4 + temp];
                }
                else
                {
                    mp += 4;
                }
            }
        }

        coop_wpn2_no = coop_wpn2_req;

        /* Crítico de la pistola especial (id 131), como WeaponSet. */
        if (((coopEquipped2() >> 16) & 0xFF) == 131)
        {
            coop_gm2 |= 0x10000000;
        }
        else
        {
            coop_gm2 &= ~0x10000000;
        }
    }

    if (coop_mounted != 0)
    {
        if (ply2.mode1 == 1)
        {
            coopStandP2();
        }

        coopApplyWeapon();
    }

    printf("[COOP] arma de P2: %d (%d B de animaciones)\n", coop_wpn2_no, msize);
    return ok;
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

    coop_p2_req = 0;
    coop_pend = 0;
    coop_lid = -1;
    coop_req_ct = 0;
    coop_unstick = 0;

    npSetMemory(coop_tgt, sizeof(coop_tgt), 0);

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

    ply2.hp = (int)sys->itm[COOP_ITM + 17];

    if ((ply2.hp < 0) || (ply2.hp > 320))
    {
        ply2.hp = (sys->gm_mode == 2) ? 320 : 160;
    }

    ply2.stflg |= sys->itm[COOP_ITM + 18] & 0x280000;
    ply2.wpnl_no = 0;

    ply2.clp_jno[0] = 0;
    ply2.clp_jno[1] = 5;
    ply2.clp_jno[2] = 9;
    ply2.clp_jno[3] = 13;
    ply2.clp_jno[4] = -1;

    ply2.cpcl = PlyCapColTab;

    ((int*)ply2.exp1)[0] = 1;
    ((short*)ply2.exp1)[34] = -1;

    ply2.mnwP = ply2.mnwPb = coop_mnw2;

    ply2.mlwP->owP[7].flg |= 0x8;
    ply2.mlwP->owP[11].flg |= 0x8;

    coopApplyWeapon();

    coopSetDmlvl();

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

    for (i = 0; i < 2; i++)
    {
        coop_wpn[i].flr_no = ply2.flr_no;
    }

    coop_mounted = 1;

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

/* Acción de P2: como el tipo 4 de bhCheckExmAtari (objeto o baúl general), sin puertas, escaleras, examinar ni salientes (hito 4). */
static void coopActionP2(void)
{
    ATR_WORK* exp;
    O_WRK* op;
    float s;
    float c;
    float px;
    float pz;
    unsigned int bits;
    int ang;
    int n;
    int i;

    njSinCos(ply2.ay, &s, &c);

    px = ply2.px - (2.0f * s);
    pz = ply2.pz - (2.0f * c);

    n = rom->etc_n + sys->metc_n;

    for (i = 0; i < n; i++)
    {
        if (i < rom->etc_n)
        {
            exp = &rom->etcp[i];
        }
        else
        {
            exp = &sys->metcp[i - rom->etc_n];
        }

        if ((!(exp->flg & 0x1)) || (exp->type != 4) || (exp->flr_no != ply2.flr_no))
        {
            continue;
        }

        if ((exp->px > px) || ((exp->px + exp->w) < px) || (exp->pz > pz) || ((exp->pz + exp->d) < pz))
        {
            continue;
        }

        ang = ply2.ay + 8192;

        if ((((ang & 0xC000) == 0x8000) && ((exp->attr & 0x400))) || (((ang & 0xC000) == 0x4000) && ((exp->attr & 0x800))) || ((!(ang & 0xC000)) && ((exp->attr & 0x1000))) || (((ang & 0xC000) == 0xC000) && ((exp->attr & 0x2000))))
        {
            continue;
        }

        coop_req_etc = i;

        if (!(exp->attr & 0x2))
        {
            op = &sys->itwp[exp->prm0];

            if (!(op->flg & 0x1))
            {
                return;
            }

            coop_req_sb = op->id;

            bits = ((exp->attr & 0x10)) ? 0x20010 : 0x10;

            if ((exp->attr & 0x1))
            {
                /* Se agacha (bhCPM2_act_cro): la petición sale al terminar la animación. */
                ply2.flg |= 0x10000;
                ply2.stflg |= 0x10000;

                ply2.mode0 = 1;
                ply2.mode1 = 0;
                ply2.mode2 = 19;
                ply2.mode3 = 0;

                coop_pend = bits;
            }
            else
            {
                coop_p2_req = bits;
            }

            return;
        }

        /* Baúles especiales A y B: solo P1. */
        if ((exp->attr & 0xC))
        {
            return;
        }

        coop_req_sb = sys->sb_id;

        if (exp->prm0 == 0xFF)
        {
            coop_p2_req = 0x40000;
        }
        else
        {
            op = &sys->obwp[exp->prm0];

            if ((op->flg & 0x1))
            {
                op->type = 100;

                *(int*)&op->mode0 = 0;

                ply2.flg |= 0x10000;
                ply2.stflg |= 0x18000;

                ply2.mode0 = 1;
                ply2.mode1 = 0;
                ply2.mode2 = 0;
                ply2.mode3 = 0;

                coop_lid = exp->prm0;
            }
        }

        return;
    }
}

/* Dentro de la ventana de P2, tras bhControlPlayer: termina las peticiones en curso o lanza una nueva. */
static void coopRequestP2(void)
{
    if (coop_unstick != 0)
    {
        coop_unstick = 0;

        if ((!(ply2.flg & 0x6)) && ((ply2.stflg & 0x18000) == 0x18000))
        {
            bhStandPlayerMotion();
        }
    }

    if (coop_pend != 0)
    {
        if ((ply2.mode0 != 1) || (ply2.mode1 != 0) || (ply2.mode2 != 19))
        {
            coop_pend = 0;
        }
        else if ((sys->cb_flg & 0x10) && (!(coop_save.cb_flg & 0x10)))
        {
            coop_p2_req = coop_pend;
            coop_pend = 0;
        }
    }

    if (coop_p2_req != 0)
    {
        /* Si no se puede abrir (P1 en una puerta, mensajes...), P2 no se queda agachado para siempre. */
        if (++coop_req_ct > 300)
        {
            printf("[COOP] petición de P2 caducada\n");

            coop_p2_req = 0;
            coop_req_ct = 0;

            /* Solo si sigue en la pose de la petición (agachado o en la tapa) y nadie le agarra. */
            if ((!(ply2.flg & 0x6)) && (((ply2.stflg & 0x18000) == 0x18000) || ((ply2.mode0 == 1) && (ply2.mode1 == 0) && (ply2.mode2 == 19))))
            {
                bhStandPlayerMotion();
            }
        }

        return;
    }

    coop_req_ct = 0;

    if ((coop_pend != 0) || (coop_lid >= 0))
    {
        return;
    }

    if ((ply2.mode0 == 1) && (!(ply2.stflg & 0x10080)) && (!(ply2.flg & 0x4000004)) && (!(sys->cb_flg & 0x4017)) && (!(coop_save.cb_flg & 0x64010)) && (!(coop_save.st_flg & 0x4)) && (!(coop_save.cb_flg & 0x28)) && (coop_pad2raw.ps & 0x200))
    {
        coopActionP2();
    }
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

        /* Muerto mientras un evento lo oculta: que su animación de muerte llegue a lanzar el game over. */
        if ((ply2.hp < 0) && (ply2.mode0 != 3))
        {
            ply2.stflg &= ~0x40000;

            *(int*)&ply2.mode0 = 3;

            /* Como player.c al forzar la muerte: un agarre deja el banco de animaciones del enemigo. */
            ply2.mnwP = ply2.mnwPb;
        }

        coop_p2_req = 0;
        coop_pend = 0;

        /* La tapa que abrió P2 no puede pedir la pantalla sin dueño (abriría el baúl de P1 en mitad del evento). */
        if ((coop_lid >= 0) && (sys->obwp[coop_lid].type == 100) && (!(ply.stflg & 0x8000)))
        {
            if (sys->obwp[coop_lid].mode0 == 3)
            {
                sys->cb_flg &= ~0x40000;
            }

            sys->obwp[coop_lid].type = 0;
            *(int*)&sys->obwp[coop_lid].mode0 = 0;
            sys->obwp[coop_lid].ax = 0;
        }

        coop_lid = -1;

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

    ply2.psh_ct = 0;
    ply2.stflg &= ~0x80;

    /* Solo durante bhControlPlayer: los objetos de P2 se actualizan después con su propio mlwP. */
    coopSwapWeaponObj();
    bhControlPlayer();
    coopSwapWeaponObj();

    coopRequestP2();

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

    /* La vida y el veneno de P2 viajan en su bloque: los copian el reintento, la máquina de escribir y la tarjeta. */
    sys->itm[COOP_ITM + 17] = (unsigned int)ply2.hp;
    sys->itm[COOP_ITM + 18] = ply2.stflg & 0x280000;

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
