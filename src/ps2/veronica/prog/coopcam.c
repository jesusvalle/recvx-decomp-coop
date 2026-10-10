#include "../../../ps2/veronica/prog/coopcam.h"

#ifdef COOP_SPLIT

#include "../../../ps2/veronica/prog/coop.h"
#include "../../../ps2/veronica/prog/ps2_NaDraw2D.h"
#include "../../../ps2/veronica/prog/ps2_NaSystem.h"
#include "../../../ps2/veronica/prog/main.h"
#include "../../../ps2/veronica/prog/cut.h"
#include "../../../ps2/veronica/prog/game.h"
#include "../../../ps2/veronica/prog/ps2_dummy.h"
#include "../../../ps2/veronica/prog/ps2_loadtim2.h"
#include "../../../ps2/veronica/prog/ps2_NaMatrix.h"
#include "../../../ps2/veronica/prog/ps2_NaView.h"
#include "../../../ps2/veronica/prog/camera.h"
#include "../../../ps2/veronica/prog/event.h"
#include "../../../ps2/veronica/prog/ps2_event.h"
#include "../../../ps2/veronica/prog/light.h"
#include "../../../ps2/veronica/prog/screen.h"
#include "../../../ps2/veronica/prog/system.h"
#include "../../../ps2/veronica/prog/ps2_NaFog.h"
#include "../../../ps2/veronica/prog/ps2_NaMem.h"

/* 1: la imagen de cada franja se desplaza con XYOFFSET_1 (3D y 2D); 0: con el centro de njSetScreen (solo 3D). */
#define COOP_SPLIT_XYOFF 0

static float coop_band[2];
static int coop_band_ok[2];
static int coop_band_cut[2];
static u_long coop_xy_base;
static u_long coop_xy_dbg[2];

static int coop_split;
static unsigned int coop_fstat[4];
static unsigned int coop_fmax;
static int coop_fslow;

static CAM_WORK coop_cam2;
static int coop_cam2_ok;
static int coop_pass = -1;
static int coop_show_p2 = 1;
static CAM_WORK coop_cambk;
static LGT_WORK coop_lgt_bak[COOP_LGT_MAX];
static unsigned char coop_lsys_bak[0xA0];
static unsigned int coop_lst_bak;
static unsigned char coop_sleep_bak;
static float coop_fog_tbl[128];
static float coop_fog_rom[128];
static float coop_fog_f[3];
static float* coop_fog_top;
static unsigned int coop_fog_col;
static int coop_fog_on;

/* Bits de primera persona de P2: gm_flg 0x40/0x80/0x800/0x2000/0x80000 (st_flg 0x800000 aparte). */
#define COOP_PE_GM 0x828C0

static unsigned int coop_pe2_gm;
static unsigned int coop_pe2_st;
static int coop_pe2_camin;
static int coop_p2fp;

/* Estado de niebla de Ninja (ps2_NaFog.c): njGenerateFogTable3 escribe la tabla, la densidad y near/far. */
extern float fNaFogNear;
extern float fNaFogFar;
extern float fNaFogDensity;
extern float fNaFogTbl[128];
extern float* fpNaFogTblTop;
extern unsigned int ulNaFogA;
extern unsigned int ulNaFogR;
extern unsigned int ulNaFogG;
extern unsigned int ulNaFogB;
extern unsigned int ulNaFogState;

/* G31 (Ps2SwapDBuff): vc = vsync que ha costado el frame. [0] = 2 o menos, [1] = 3, [2] = 4, [3] = 5 o más. */
void coopFrameStat(unsigned int vc)
{
    int i;

    if (vc <= 2)
    {
        i = 0;
    }
    else if (vc >= 5)
    {
        i = 3;
    }
    else
    {
        i = vc - 2;
    }

    coop_fstat[i]++;

    if (vc > coop_fmax)
    {
        coop_fmax = vc;
    }

    coop_fslow = (vc > 2) ? 1 : 0;
}

static void coopDrawRect(float x0, float y0, float x1, float y1, unsigned int argb)
{
    NJS_POINT2COL p2c;
    NJS_POINT2 p[4];
    NJS_COLOR col[4];

    njColorBlendingMode(0, 8);
    njColorBlendingMode(1, 6);

    p2c.p = p;
    p2c.col = col;
    p2c.tex = NULL;
    p2c.num = 1;

    col[0].color = argb;
    col[1].color = argb;
    col[2].color = argb;
    col[3].color = argb;

    p[0].x = x0;
    p[0].y = y0;
    p[1].x = x1;
    p[1].y = y0;
    p[2].x = x1;
    p[2].y = y1;
    p[3].x = x0;
    p[3].y = y1;

    njDrawPolygon2D(&p2c, 4, -0.8f, 96);
}

/* G30, después del bloque de dibujo: cuadradito rojo si el último frame pasó de 2 vsync. */
void coopDrawMeter(void)
{
    if (coop_fslow != 0)
    {
        coopDrawRect(624.0f, 8.0f, 632.0f, 16.0f, 0xFFFF0000);
    }
}

void coopCamRoomStart(void)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        coop_fstat[i] = 0;
    }

    coop_fmax = 0;
    coop_fslow = 0;

    coop_band_ok[0] = 0;
    coop_band_ok[1] = 0;

    coop_cam2_ok = 0;

    coop_pe2_gm = 0;
    coop_pe2_st = 0;
}

/* L1+L2+R1+R2 del mando 1 (botones SCE de p1per: L2 0x1, R2 0x2, L1 0x4, R1 0x8), al pulsar el último. */
static void coopCamToggle(void)
{
    const PDS_PERIPHERAL* pp;

    pp = sys->p1per;

    if ((pp == NULL) || (!(sys->sp_flg & 0x20)))
    {
        return;
    }

    if (((pp->on & 0xF) != 0xF) || (!(pp->press & 0xF)))
    {
        return;
    }

    /* Solo en juego normal: sin subpantallas, mensajes, jugador ocupado, cinemática ni puerta. */
    if ((sys->st_flg & 0x1C04020C) || (sys->cb_flg & 0x5))
    {
        return;
    }

    sys->itm[COOP_CAM_OPT] = (sys->itm[COOP_CAM_OPT] != 0) ? 0 : 1;

    if (sys->itm[COOP_CAM_OPT] != 0)
    {
        coop_pe2_gm = (coop_pe2_gm & ~0x20C0) | 0x800;
        coop_pe2_st = 0;
    }

    coop_cam2_ok = 0;

    coop_band_ok[0] = 0;
    coop_band_ok[1] = 0;
}

static int coopSplitWanted(void)
{
    if (sys->itm[COOP_CAM_OPT] != 0)
    {
        return 0;
    }

    if (coopP2Active() == 0)
    {
        return 0;
    }

    if ((sys->cb_flg & 0x5) || (sys->st_flg & 0x1) || (sys->cine_an > 0) || (sys->gm_flg & 0x100))
    {
        return 0;
    }

    if (rom->lgt_n > COOP_LGT_MAX)
    {
        return 0;
    }

    return 1;
}

/* Escribe un registro del GS al momento, como Ps2SetFogColor (ps2_dummy.c). */
static void coopGsSet(u_long data, u_long reg)
{
    D2_SyncTag();

    ((u_long*)WORKBASE)[0] = DMAend | 0x2;
    ((u_long*)WORKBASE)[1] = 0;

    ((u_long*)WORKBASE)[2] = SCE_GIF_SET_TAG(1, SCE_GS_TRUE, SCE_GS_FALSE, 0, SCE_GIF_PACKED, 1);
    ((u_long*)WORKBASE)[3] = SCE_GIF_PACKED_AD;

    ((u_long*)WORKBASE)[4] = data;
    ((u_long*)WORKBASE)[5] = reg;

    loadImage((void*)0xF0000000);

    D2_SyncTag();
}

/* Envía lo dibujado hasta ahora, como el inventario antes de cambiar el recorte (sub1.c:3540-3580). */
static void coopFlush(void)
{
    SyncPath();

    Ps2DrawOTag();
    Ps2ClearOT();

    SyncPath();
}

/* XYOFFSET del entorno de dibujo (los dos búferes se guardan en coop_xy_dbg para comprobar que coinciden). */
static u_long coopXyBase(void)
{
    coop_xy_dbg[0] = *(u_long*)&Db.draw01.xyoffset1;
    coop_xy_dbg[1] = *(u_long*)&Db.draw11.xyoffset1;

    return coop_xy_dbg[0];
}

/* Franja s (0 arriba, 1 abajo): recorte en sus 240 filas y la fila k del plano de 640x480 en su borde superior. */
static void coopStripBegin(int s, float k)
{
#if COOP_SPLIT_XYOFF
    int ofy;
#else
    NJS_SCREEN scr;
#endif
    u_long sc;

    coopFlush();

    sc = SCE_GS_SET_SCISSOR(0, 639, s * 240, (s * 240) + 239);

    Ps2_gs_save.SCISSOR = sc;

    coopGsSet(sc, SCE_GS_SCISSOR_1);

#if COOP_SPLIT_XYOFF
    ofy = (int)((coop_xy_base >> 32) & 0xFFFF) + (int)((k - (s * 240)) * 16.0f);

    coopGsSet(SCE_GS_SET_XYOFFSET(coop_xy_base & 0xFFFF, ofy & 0xFFFF), SCE_GS_XYOFFSET_1);
#else
    scr.dist = _nj_screen_.dist;
    scr.w = 640.0f;
    scr.h = 480.0f;
    scr.cx = 320.0f;
    scr.cy = (240.0f - k) + (s * 240);

    njSetScreen(&scr);
#endif
}

static void coopStripEnd(void)
{
#if !COOP_SPLIT_XYOFF
    NJS_SCREEN scr;
#endif

    coopFlush();

#if COOP_SPLIT_XYOFF
    coopGsSet(coop_xy_base, SCE_GS_XYOFFSET_1);
#else
    scr.dist = _nj_screen_.dist;
    scr.w = 640.0f;
    scr.h = 480.0f;
    scr.cx = 320.0f;
    scr.cy = 240.0f;

    njSetScreen(&scr);
#endif

    njUserClipping(0, NULL);
}

/* bhDrawScope dibuja en coordenadas fijas de 640x480 (centro en la fila 240): con njSetScreen no se desplaza,
   así que solo durante la mira se mueve XYOFFSET para que su centro caiga en el de la franja s. */
static void coopDrawScopeStrip(int s)
{
#if COOP_SPLIT_XYOFF
    bhDrawScope();
#else
    int ofy;

    coopFlush();

    ofy = (int)((coop_xy_base >> 32) & 0xFFFF) + ((120 - (s * 240)) * 16);

    coopGsSet(SCE_GS_SET_XYOFFSET(coop_xy_base & 0xFFFF, ofy & 0xFFFF), SCE_GS_XYOFFSET_1);

    bhDrawScope();

    coopFlush();

    coopGsSet(coop_xy_base, SCE_GS_XYOFFSET_1);
#endif
}

/* Fila superior k (0-240) de la ventana de la franja s: sigue el punto que sigue la cámara (cam.ply = gpy + ci->h).
   Se llama con la cámara de la franja ya construida (cam.mtx) y la pantalla centrada. */
static float coopBand(int s, BH_PWORK* pw, int fp)
{
    CAM_WRK* ci;
    NJS_POINT3 pos;
    NJS_POINT3 p3;
    NJS_POINT2 p2;
    float t;

    if (fp != 0)
    {
        t = 120.0f;
    }
    else
    {
        ci = &rom->cutp[cam.ncut].cam[cam.camver];

        pos.x = pw->gpx;
        pos.y = pw->gpy + ci->h;
        pos.z = pw->gpz;

        njCalcPoint(cam.mtx, &pos, &p3);

        if (p3.z > -1.0f)
        {
            t = 120.0f;
        }
        else
        {
            njProjectScreen(cam.mtx, &pos, &p2);

            t = ((p2.y - fNaViwOffsetY) + 240.0f) - 120.0f;
        }

        if (t < 0)
        {
            t = 0;
        }

        if (t > 240.0f)
        {
            t = 240.0f;
        }
    }

    if ((coop_band_ok[s] == 0) || (coop_band_cut[s] != cam.ncut))
    {
        coop_band[s] = t;

        coop_band_ok[s] = 1;
        coop_band_cut[s] = cam.ncut;
    }
    else
    {
        coop_band[s] += 0.25f * (t - coop_band[s]);
    }

    return coop_band[s];
}

/* Mallas, luces y recorte de vista del plano de la cam actual. fp: vista en primera persona de who (como bhInitPlEyeCamera).
   back: también el color de fondo (solo para P1: la franja de P2 usa el de P1). */
static void coopApplyCamState(int fp, BH_PWORK* who, int back)
{
    NJS_CNK_OBJECT* obj;
    LGT_WORK* lp;
    int i;

    if (fp != 0)
    {
        for (i = 0; i < (int)rom->mdl.obj_num; i++)
        {
            obj = &rom->mdl.objP[i];

            if (obj->model != NULL)
            {
                obj->evalflags &= ~0x8;
            }
        }

        lp = &rom->lgtp[4];

        for (i = 4; i < rom->lgt_n; i++, lp++)
        {
            lp->flg &= ~0x2;
        }

        if ((who->wpnr_no == 10) || (who->wpnr_no == 19))
        {
            njClipZ(-1.1f, -20000.0f);
        }
        else
        {
            njClipZ(-1.0f, -20000.0f);
        }

        return;
    }

    bhSetHideObjLgt(cam.ncut);

    /* Como bhSetCut: solo con el inventario cerrado. */
    if ((sys->ts_flg & 0x200))
    {
        bhChangeViewClipRM();
        bhChangeClipVolumeRM();

        if (back != 0)
        {
            bhChangeBackColor();
        }
    }
}

/* bhControlLight anima las luces en cada llamada: la pasada de P2 se hace sobre una copia que después se descarta. */
static void coopLightSave(void)
{
    njMemCopy(coop_lgt_bak, rom->lgtp, rom->lgt_n * sizeof(LGT_WORK));
    njMemCopy(coop_lsys_bak, &sys->lgtp, (unsigned char*)&sys->mes_idx - (unsigned char*)&sys->lgtp);

    coop_lst_bak = sys->st_flg;
    coop_sleep_bak = pl_sleep_cnt;
}

static void coopLightRestore(void)
{
    njMemCopy(rom->lgtp, coop_lgt_bak, rom->lgt_n * sizeof(LGT_WORK));
    njMemCopy(&sys->lgtp, coop_lsys_bak, (unsigned char*)&sys->mes_idx - (unsigned char*)&sys->lgtp);

    sys->st_flg = (sys->st_flg & ~0x1000000) | (coop_lst_bak & 0x1000000);
    pl_sleep_cnt = coop_sleep_bak;
}

/* Niebla del plano de la cam actual (la de P2), si la sala tiene niebla y la de P1 (p1) es distinta. */
static void coopFogApply(CAM_WORK* p1)
{
    unsigned int col;

    coop_fog_on = 0;

    if ((!(sys->st_flg & 0x2)) || (ulNaFogState == 0))
    {
        return;
    }

    if ((cam.fog_nr == p1->fog_nr) && (cam.fog_fr == p1->fog_fr) && (cam.fog_col == p1->fog_col))
    {
        return;
    }

    coop_fog_on = 1;

    njMemCopy(coop_fog_tbl, fNaFogTbl, sizeof(coop_fog_tbl));
    njMemCopy(coop_fog_rom, rom->fog, sizeof(coop_fog_rom));

    coop_fog_f[0] = fNaFogNear;
    coop_fog_f[1] = fNaFogFar;
    coop_fog_f[2] = fNaFogDensity;

    coop_fog_top = fpNaFogTblTop;

    coop_fog_col = (ulNaFogA << 24) | (ulNaFogR << 16) | (ulNaFogG << 8) | ulNaFogB;

    /* Como sync.c al refrescar la niebla de un plano. */
    if (!(sys->st_flg & 0x100000))
    {
        col = cam.fog_col;
    }
    else
    {
        col = sys->fog_col & 0xFFFFFF;
    }

    njSetFogColor(col);
    njGenerateFogTable3(rom->fog, cam.fog_nr, cam.fog_fr);
    njSetFogTable(rom->fog);
}

static void coopFogRestore(void)
{
    if (coop_fog_on == 0)
    {
        return;
    }

    njMemCopy(fNaFogTbl, coop_fog_tbl, sizeof(coop_fog_tbl));
    njMemCopy(rom->fog, coop_fog_rom, sizeof(coop_fog_rom));

    fNaFogNear = coop_fog_f[0];
    fNaFogFar = coop_fog_f[1];
    fNaFogDensity = coop_fog_f[2];

    fpNaFogTblTop = coop_fog_top;

    njSetFogColor(coop_fog_col);

    coop_fog_on = 0;
}

/* La pasada de espejo del bloque original (game.c), para la franja actual. */
static void coopMirror(void)
{
    if ((sys->st_flg & 0x100))
    {
        sys->gm_flg |= 0x4000;

        njSetMatrix(cam.mtxb, cam.mtx);

        njMirror(cam.mtx, &sys->mr_pl);

        bhSetLight();

        bhEtcMirrorDrawModel();

        njSetMatrix(cam.mtx, cam.mtxb);

        sys->gm_flg &= ~0x4000;
    }
}

/* G29 (bhMainSequence, tras bhCheckCut): la cámara de P2 con el código original, en un cambio de contexto. */
void coopUpdateCamera2(void)
{
    BH_PWORK* plp_bak;
    unsigned int gm;
    unsigned int st;
    unsigned int pt;
    unsigned int ef;
    int fog_ct;
    short fil_no;
    short fil_rt;
    unsigned int rfog_col;
    float rfog_nr;
    float rfog_fr;
    int p1fp;

    coopCamToggle();

    coop_split = coopSplitWanted();

    if (coop_split == 0)
    {
        coop_cam2_ok = 0;

        coop_band_ok[0] = 0;
        coop_band_ok[1] = 0;

        /* Sin franja, P2 no puede quedarse en primera persona. */
        coop_pe2_gm = (coop_pe2_gm & ~0x20C0) | ((coop_pe2_gm & 0x40) ? 0x800 : 0);
        coop_pe2_st = 0;
        return;
    }

    p1fp = (sys->gm_flg & 0x40) ? 1 : 0;

    coop_cambk = cam;
    plp_bak = plp;

    gm = sys->gm_flg;
    st = sys->st_flg;
    pt = sys->pt_flg;
    ef = sys->ef_flg;

    fog_ct = sys->fog_ct;
    fil_no = sys->fil_no;
    fil_rt = sys->fil_rt;

    rfog_col = rom->fog_col;
    rfog_nr = rom->fog_nr;
    rfog_fr = rom->fog_fr;

    if (coop_cam2_ok == 0)
    {
        coop_cam2 = cam;
    }

    cam = coop_cam2;
    plp = &ply2;

    /* El modo de vista de P2 (no el de P1), sin plano fijo. Mismo orden que bhMainSequence (game.c). */
    sys->gm_flg = (sys->gm_flg & ~(COOP_PE_GM | 0x1000)) | coop_pe2_gm;
    sys->st_flg = (sys->st_flg & ~0x800000) | coop_pe2_st;

    if ((sys->gm_flg & 0x2000))
    {
        bhInitPlEyeCamera();
    }

    bhCheckCut(((coop_cam2_ok == 0) || (sys->gm_flg & 0x800)) ? 1 : 0);

    if ((sys->gm_flg & 0x40) && (sys->sp_flg & 0x40))
    {
        bhSetPlEyeCamera();
    }

    coop_pe2_gm = sys->gm_flg & COOP_PE_GM;
    coop_pe2_st = sys->st_flg & 0x800000;

    coop_cam2_ok = 1;

    coop_cam2 = cam;

    plp = plp_bak;
    cam = coop_cambk;

    sys->gm_flg = gm;
    sys->st_flg = st;
    sys->pt_flg = pt;
    sys->ef_flg = ef;

    sys->fog_ct = fog_ct;
    sys->fil_no = fil_no;
    sys->fil_rt = fil_rt;

    rom->fog_col = rfog_col;
    rom->fog_nr = rfog_nr;
    rom->fog_fr = rfog_fr;

    coopApplyCamState(p1fp, &ply, 1);
}

/* G30: las dos franjas. 1 si ha dibujado (el bloque original no se ejecuta). */
int coopSplitDraw(void)
{
    unsigned int pt;
    unsigned int gm;
    unsigned int sp;
    int p1fp;
    int p2fp;
    float k;

    if ((coop_split == 0) || (coop_cam2_ok == 0))
    {
        return 0;
    }

    /* Un guion o examinar (tarea 8) puede haber empezado después de G29: su primer fotograma ya va a pantalla completa,
       sin que la vuelta al estado de P1 pise las mallas, el recorte y el fondo de la cámara de evento. */
    if (coopSplitWanted() == 0)
    {
        coop_split = 0;
        coop_cam2_ok = 0;

        coop_band_ok[0] = 0;
        coop_band_ok[1] = 0;
        return 0;
    }

    p1fp = (sys->gm_flg & 0x40) ? 1 : 0;

    pt = sys->pt_flg;
    gm = sys->gm_flg;
    sp = sys->sp_flg;

    coop_xy_base = coopXyBase();

    if ((sys->gm_flg & 0x200))
    {
        bhDrawSmallScreenRenderTexture();
    }

    p2fp = (coop_pe2_gm & 0x40) ? 1 : 0;

    coop_p2fp = p2fp;

    /* P2 se ve si el juego dibuja al jugador, o si no lo dibuja solo porque P1 está en primera persona. */
    coop_show_p2 = ((pt & 0x1) || (p1fp != 0)) ? 1 : 0;

    /* Franja de P1: su cámara y su estado ya están puestos. */
    coop_pass = 0;

    k = coopBand(0, &ply, p1fp);

    coopStripBegin(0, k);

    bhAllDrawModel();

    coopMirror();

    if ((sys->st_flg & 0x800000))
    {
        coopDrawScopeStrip(0);
    }

    coopStripEnd();

    /* Franja de P2. */
    coop_cambk = cam;

    coopLightSave();

    cam = coop_cam2;

    /* Temblor de cámara (terremotos, explosiones): el actual, también en la franja de P2. */
    cam.ofx = coop_cambk.ofx;
    cam.ofy = coop_cambk.ofy;
    cam.ofz = coop_cambk.ofz;

    sys->gm_flg &= ~0xC0;

    if (p2fp != 0)
    {
        sys->gm_flg |= 0xC0;
    }

    sys->pt_flg = (pt & ~0x1) | coop_show_p2;

    /* Varios efectos avanzan al dibujarse si sp_flg & 0x8 (bhDraw022/024/025/027/107/134, la lluvia de bhEff106 con rand()):
       en la segunda pasada solo se dibujan. */
    sys->sp_flg &= ~0x8;

    coop_pass = 1;

    bhControlCamera();

    coopApplyCamState(p2fp, &ply2, 0);

    bhControlLight();
    bhSetLight();

    coopFogApply(&coop_cambk);

    k = coopBand(1, &ply2, p2fp);

    coopStripBegin(1, k);

    bhAllDrawModel();

    coopMirror();

    if ((p2fp != 0) && (coop_pe2_st & 0x800000))
    {
        coopScopeSwap();
        coopDrawScopeStrip(1);
        coopScopeSwap();
    }

    coopStripEnd();

    /* Vuelta al estado de P1. */
    coop_pass = -1;

    cam = coop_cambk;

    sys->gm_flg = gm;
    sys->pt_flg = pt;
    sys->sp_flg = sp;

    coopFogRestore();
    coopLightRestore();

    bhControlCamera();

    coopApplyCamState(p1fp, &ply, 1);

    bhSetLight();

    coopDrawRect(0.0f, 239.0f, 640.0f, 241.0f, 0xFF000000);

    if ((sys->st_flg & 0x40000000))
    {
        bhDrawThermometer();
    }

    if (sys->fade_an > 0)
    {
        bhDrawScreenFade();
    }

    return 1;
}

/* coopDrawPlayer2: si P2 se dibuja en la pasada actual. Fuera de una pasada, como el original. */
int coopSplitShowP2(void)
{
    if (coop_pass < 0)
    {
        return (sys->pt_flg & 0x1) ? 1 : 0;
    }

    if ((coop_pass == 1) && (coop_p2fp != 0))
    {
        return 0;
    }

    return coop_show_p2;
}

/* G32, al final de coopBegin: P2 ve su modo de vista; en primera persona, también su cámara de ojos. */
void coopCamP2Begin(void)
{
    sys->gm_flg = (sys->gm_flg & ~COOP_PE_GM) | coop_pe2_gm;
    sys->st_flg = (sys->st_flg & ~0x800000) | coop_pe2_st;

    if ((coop_pe2_gm & 0x40))
    {
        cam = coop_cam2;

        coop_pe2_camin = 1;
    }
    else
    {
        cam.pe_ax = coop_cam2.pe_ax;
        cam.pe_pers = coop_cam2.pe_pers;

        coop_pe2_camin = 0;
    }
}

/* G32, al principio de coopEnd (antes de que restaure los flags y cam de P1). */
void coopCamP2End(void)
{
    coop_pe2_gm = sys->gm_flg & COOP_PE_GM;
    coop_pe2_st = sys->st_flg & 0x800000;

    if (coop_pe2_camin != 0)
    {
        coop_cam2 = cam;
    }
    else
    {
        coop_cam2.pe_ax = cam.pe_ax;
        coop_cam2.pe_pers = cam.pe_pers;
    }

    coop_pe2_camin = 0;

    /* Con una cámara no hay franja para P2: apunta en tercera persona. */
    if (sys->itm[COOP_CAM_OPT] != 0)
    {
        coop_pe2_gm &= ~0x2000;
    }
}

#endif
