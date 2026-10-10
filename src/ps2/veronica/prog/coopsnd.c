#include "../../../ps2/veronica/prog/coop.h"

#ifdef COOP

/* Sonidos de arma de P2 (ver docs/architecture/world-systems.md, "Driver de sonido").
 *
 * Solo cabe un banco de armas (banco de SE 1, puerto 4 del IOP). Los sonidos que usa el
 * jugador con el arma de P2 se sacan de su ARMS_xxx.SPQ y se suben a un hueco libre de la
 * SPU2 (0x1E7400, Tsnd_spuadr_tbl[8], justo tras la zona del puerto 7). Sus programas se
 * añaden al HD del banco de SE 4 (voz, CORE_xxx), en las listas 32 + lista, con muestras
 * que apuntan al hueco (los desplazamientos de Vagi son relativos a la zona del puerto 7).
 *
 * El banco de P2 se pide por la misma vía que los demás (SpqFileReadRequestFlag), con un
 * valor propio. LoadSoundPackFile corre en la interrupción de VSync: coopSePack no imprime.
 *
 * Voz de P2 (CORE_<id>.SPQ, 4 programas: 0x400 daño por detrás, 0x401 muerte, 0x402 daño leve,
 * 0x403 daño fuerte). Su BD entero va al hueco bajo de la SPU2 (0x050A0-0x1521F): la carga usa
 * el puerto 7 con un desplazamiento inicial de 0x2B4A0 (G28, ps2_sg_sd.c), así que el DMA escribe
 * en 0x2050A0 y depende de que la SPU2 dé la vuelta a los 2 MB (PCSX2 enmascara TSA con 0xFFFFF;
 * en hardware real no está comprobado). Sus Vagi pasan a 0xFFE2B4A0 + desplazamiento: modhsyn suma
 * base + desplazamiento en 32 bits y 0x1D9C00 + 0xFFE2B4A0 da exactamente 0x050A0 (la reproducción
 * no depende de la vuelta). Sus programas van a las listas 4-7 del banco 4 (G23, CallPlayerVoice).
 *
 * HD fusionado del banco 4, por índices (sset = sample = muestra, como espera el driver):
 * voz de P1 | armas de P2 | entrada falsa | voz de P2. El driver calcula la duración de cada muestra
 * como Vagi[i + 1] - Vagi[i] y la de la última con el tamaño del BD de Head: la entrada falsa, que
 * ningún programa usa, marca el final de las armas (o de la voz de P1) antes del salto a 0xFFE2B4A0,
 * y Head lleva 0xFFE2B4A0 + tamaño de la voz de P2.
 *
 * La voz de P2 se carga siempre con P2 (aunque sea el personaje de P1): el CORE de P1 puede cambiar
 * después (Chris en la historia, mercenarios) y la carga de P1 vuelve a fusionar. G23 solo remapea
 * si el personaje de la voz de P2 no es el del último CORE de P1. */

#include <stdio.h>
#include "../../../ps2/veronica/prog/player.h"
#include "../../../ps2/veronica/prog/sdfunc.h"
#include "../../../ps2/veronica/prog/sdc.h"
#include "../../../ps2/veronica/prog/macros.h"
#include "../../../ps2/veronica/prog/main.h"

#define COOP_SE_BANK 4
#define COOP_SE_PORT 8
#define COOP_SE_BASE 0xD800
#define COOP_SE_MAX 0xDDC0
#define COOP_SE_PROG 32
#define COOP_SE_REQ 5
#define COOP_SE_HD 4096
#define COOP_SE_CORE 1024
#define COOP_SE_N 16
#define COOP_SE_PSZ 80
#define COOP_HD_VAGI 8
#define COOP_HD_SMPL 42
#define COOP_HD_SSET 6
#define COOP_VO_REQ 6
#define COOP_VO_N 8
#define COOP_VO_PROG 4
#define COOP_VO_LIST 4
#define COOP_VO_OFF 0xFFE2B4A0
#define COOP_VO_TRANS 0x2B4A0
#define COOP_VO_MAX 0x10180

extern short SE_BANK[5];
extern int SpqFileReadRequestFlag;
extern unsigned short SpqKeyCode;

static unsigned char coop_se_hd[COOP_SE_HD] __attribute__((aligned(64)));
static unsigned char coop_core_hd[COOP_SE_CORE] __attribute__((aligned(64)));
static int coop_core_ok;
static int coop_core_sz;
static unsigned char coop_p2_head[0x50];
static unsigned char coop_p2_vag[COOP_SE_N][COOP_HD_VAGI];
static unsigned char coop_p2_smp[COOP_SE_N][COOP_HD_SMPL];
static unsigned char coop_p2_ss[COOP_SE_N][COOP_HD_SSET];
static unsigned char coop_p2_prog[COOP_SE_PROG][COOP_SE_PSZ];
static int coop_p2_psz[COOP_SE_PROG];
static int coop_p2_n;
static unsigned int coop_p2_bdsz;
static unsigned char* coop_ent[COOP_SE_PROG * 2];
static int coop_esz[COOP_SE_PROG * 2];
static int coop_se_state;
static int coop_se_built;
static int coop_se_want = -1;
static int coop_se_cur = -1;
static int coop_se_swap;
static short coop_se_bank_bak;
static unsigned char coop_vo_head[0x50];
static unsigned char coop_vo_vag[COOP_VO_N][COOP_HD_VAGI];
static unsigned char coop_vo_smp[COOP_VO_N][COOP_HD_SMPL];
static unsigned char coop_vo_ss[COOP_VO_N][COOP_HD_SSET];
static unsigned char coop_vo_prog[COOP_VO_PROG][COOP_SE_PSZ];
static int coop_vo_psz[COOP_VO_PROG];
static int coop_vo_n;
static int coop_vo_ok;
static unsigned int coop_vo_bdsz;
static int coop_vo_state;
static int coop_vo_built;
static int coop_vo_want = -1;
static int coop_vo_cur = -1;
static int coop_vo_id = -1;
static int coop_vo_p1 = -1;
static int coop_vo_in;
static int coop_mrg_vo;
static unsigned int coop_bd_off;
static unsigned char coop_dum_vag[COOP_HD_VAGI];
static unsigned char coop_dum_smp[COOP_HD_SMPL];
static unsigned char coop_dum_ss[COOP_HD_SSET];

/* Listas que pide el jugador (player.c, weapon.c): disparo, bombeo, cargador, corredera, sin munición, cuchillo... */
static const unsigned char coop_se_order[15] = { 5, 15, 1, 2, 4, 6, 7, 8, 9, 16, 19, 20, 21, 29, 30 };

static unsigned int coopRd32(const unsigned char* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned int)p[3] << 24);
}

static int coopRd16(const unsigned char* p)
{
    return p[0] | (p[1] << 8);
}

static void coopWr32(unsigned char* p, unsigned int v)
{
    p[0] = v;
    p[1] = v >> 8;
    p[2] = v >> 16;
    p[3] = v >> 24;
}

static void coopWr16(unsigned char* p, int v)
{
    p[0] = v;
    p[1] = v >> 8;
}

static void coopCopy(unsigned char* d, const unsigned char* s, int n)
{
    while (n-- > 0)
    {
        *d++ = *s++;
    }
}

/* Chunks del HD (formato JAM): Head (hd + 0x10) da su posición; 0 Prog, 1 Sset, 2 Smpl, 3 Vagi. */
static unsigned char* coopHdChunk(unsigned char* hd, int k)
{
    return &hd[coopRd32(&hd[0x24 + (k * 4)])];
}

static int coopHdNum(unsigned char* c)
{
    return coopRd32(&c[12]) + 1;
}

static unsigned char* coopHdEnt(unsigned char* c, int i)
{
    unsigned int o;

    if ((i < 0) || (i >= coopHdNum(c)))
    {
        return NULL;
    }

    o = coopRd32(&c[16 + (i * 4)]);

    return (o == 0xFFFFFFFF) ? NULL : &c[o];
}

/* Sset -> Smpl -> Vagi. Devuelve el índice de la muestra (o -1) y el sample en *m. */
static int coopHdVag(unsigned char* hd, int s, int* m)
{
    unsigned char* e;

    e = coopHdEnt(coopHdChunk(hd, 1), s);

    if ((e == NULL) || (e[3] == 0))
    {
        return -1;
    }

    *m = coopRd16(&e[4]);

    e = coopHdEnt(coopHdChunk(hd, 2), *m);

    if ((e == NULL) || (coopHdEnt(coopHdChunk(hd, 3), coopRd16(e)) == NULL))
    {
        return -1;
    }

    return coopRd16(e);
}

/* Elige los programas de P2 que caben, compacta sus muestras al principio del BD y guarda sus
 * entradas con índices locales (0..coop_p2_n-1; coopSeMerge les suma los del banco de voz). */
static int coopSeBuildP2(unsigned char* hd, unsigned char* bd, unsigned int bdsz)
{
    unsigned char* prog;
    unsigned char* vagi;
    unsigned char* e;
    unsigned int v_off[COOP_SE_N];
    unsigned int v_size[COOP_SE_N];
    unsigned int v_dst[COOP_SE_N];
    unsigned char v_used[COOP_SE_N];
    unsigned char v_tmp[COOP_SE_N];
    signed char ss_new[COOP_SE_N];
    unsigned char ss_used[COOP_SE_N];
    unsigned char done[COOP_SE_PROG];
    int nv;
    int np;
    int nss;
    int i;
    int j;
    int k;
    int p;
    int s;
    int v;
    int m;
    int ns;
    int ssz;
    int soff;
    int ok;
    unsigned int total;
    unsigned int need;
    unsigned int cur;
    unsigned int best;

    prog = coopHdChunk(hd, 0);
    vagi = coopHdChunk(hd, 3);

    np = coopHdNum(prog);
    nss = coopHdNum(coopHdChunk(hd, 1));
    nv = coopHdNum(vagi);

    if ((nv > COOP_SE_N) || (nss > COOP_SE_N))
    {
        return 0;
    }

    if (np > COOP_SE_PROG)
    {
        np = COOP_SE_PROG;
    }

    for (i = 0; i < nv; i++)
    {
        e = coopHdEnt(vagi, i);

        v_off[i] = (e != NULL) ? coopRd32(e) : 0xFFFFFFFF;
        v_used[i] = 0;
    }

    for (i = 0; i < nv; i++)
    {
        best = bdsz;

        for (j = 0; j < nv; j++)
        {
            if ((v_off[j] != 0xFFFFFFFF) && (v_off[j] > v_off[i]) && (v_off[j] < best))
            {
                best = v_off[j];
            }
        }

        v_size[i] = (v_off[i] < best) ? best - v_off[i] : 0;
    }

    for (i = 0; i < COOP_SE_N; i++)
    {
        ss_used[i] = 0;
        ss_new[i] = -1;
    }

    for (i = 0; i < COOP_SE_PROG; i++)
    {
        done[i] = 0;
        coop_p2_psz[i] = 0;
    }

    total = 0;

    /* Primero las listas del jugador, después las demás mientras quepan. */
    for (k = 0; k < (15 + COOP_SE_PROG); k++)
    {
        p = (k < 15) ? coop_se_order[k] : k - 15;

        if ((p >= np) || (done[p] != 0))
        {
            continue;
        }

        done[p] = 1;

        e = coopHdEnt(prog, p);

        if (e == NULL)
        {
            continue;
        }

        soff = coopRd32(e);
        ns = e[4];
        ssz = e[5];

        if ((ns == 0) || ((soff + (ns * ssz)) > COOP_SE_PSZ))
        {
            continue;
        }

        for (i = 0; i < nv; i++)
        {
            v_tmp[i] = 0;
        }

        ok = 1;
        need = 0;

        for (j = 0; j < ns; j++)
        {
            s = coopRd16(&e[soff + (j * ssz)]);
            v = coopHdVag(hd, s, &m);

            if ((v < 0) || (v >= nv) || (v_size[v] == 0))
            {
                ok = 0;
                break;
            }

            if ((v_used[v] == 0) && (v_tmp[v] == 0))
            {
                v_tmp[v] = 1;
                need += v_size[v];
            }
        }

        if ((ok == 0) || (ALIGN_UP(total + need, 64) > COOP_SE_MAX))
        {
            continue;
        }

        for (j = 0; j < ns; j++)
        {
            s = coopRd16(&e[soff + (j * ssz)]);

            ss_used[s] = 1;
            v_used[coopHdVag(hd, s, &m)] = 1;
        }

        total += need;

        coop_p2_psz[p] = soff + (ns * ssz);

        coopCopy(coop_p2_prog[p], e, coop_p2_psz[p]);
    }

    /* Compacta las muestras elegidas por orden de dirección (el destino nunca pasa al origen). */
    cur = 0;

    for (;;)
    {
        v = -1;

        for (i = 0; i < nv; i++)
        {
            if ((v_used[i] == 1) && ((v < 0) || (v_off[i] < v_off[v])))
            {
                v = i;
            }
        }

        if (v < 0)
        {
            break;
        }

        v_used[v] = 2;
        v_dst[v] = cur;

        coopCopy(&bd[cur], &bd[v_off[v]], v_size[v]);

        cur += v_size[v];
    }

    /* Índices nuevos (sset = sample = muestra, como espera el driver) por orden de dirección. */
    coop_p2_n = 0;

    for (;;)
    {
        s = -1;

        for (i = 0; i < nss; i++)
        {
            if ((ss_used[i] != 0) && (ss_new[i] < 0) && ((s < 0) || (v_off[coopHdVag(hd, i, &m)] < v_off[coopHdVag(hd, s, &m)])))
            {
                s = i;
            }
        }

        if (s < 0)
        {
            break;
        }

        k = coop_p2_n++;

        ss_new[s] = k;

        v = coopHdVag(hd, s, &m);

        coopCopy(coop_p2_vag[k], coopHdEnt(vagi, v), COOP_HD_VAGI);
        coopWr32(coop_p2_vag[k], COOP_SE_BASE + v_dst[v]);

        coopCopy(coop_p2_smp[k], coopHdEnt(coopHdChunk(hd, 2), m), COOP_HD_SMPL);
        coopWr16(coop_p2_smp[k], k);

        coopCopy(coop_p2_ss[k], coopHdEnt(coopHdChunk(hd, 1), s), COOP_HD_SSET);
        coop_p2_ss[k][3] = 1;
        coopWr16(&coop_p2_ss[k][4], k);
    }

    for (p = 0; p < COOP_SE_PROG; p++)
    {
        if (coop_p2_psz[p] != 0)
        {
            e = coop_p2_prog[p];

            for (j = 0; j < e[4]; j++)
            {
                s = coopRd16(&e[coopRd32(e) + (j * e[5])]);

                coopWr16(&e[coopRd32(e) + (j * e[5])], ss_new[s]);
            }
        }
    }

    coopCopy(coop_p2_head, hd, 0x50);

    coop_p2_bdsz = ALIGN_UP(cur, 64);

    return coop_p2_n;
}

/* Escribe un chunk con las entradas de coop_ent/coop_esz (NULL = vacía). Devuelve la nueva posición o -1. */
static int coopSeChunk(int pos, const char* tag, int n)
{
    unsigned char* c;
    int i;
    int o;

    c = &coop_se_hd[pos];
    o = 16 + (n * 4);

    if ((pos + o) > COOP_SE_HD)
    {
        return -1;
    }

    coopCopy(c, (const unsigned char*)tag, 8);

    for (i = 0; i < n; i++)
    {
        if (coop_ent[i] == NULL)
        {
            coopWr32(&c[16 + (i * 4)], 0xFFFFFFFF);
            continue;
        }

        if ((pos + o + coop_esz[i]) > COOP_SE_HD)
        {
            return -1;
        }

        coopWr32(&c[16 + (i * 4)], o);
        coopCopy(&c[o], coop_ent[i], coop_esz[i]);

        o += coop_esz[i];
    }

    while ((o & 15) != 0)
    {
        if ((pos + o) >= COOP_SE_HD)
        {
            return -1;
        }

        c[o++] = 0xFF;
    }

    coopWr32(&c[8], o);
    coopWr32(&c[12], n - 1);

    return pos + o;
}

/* Voz de P2: copia sus entradas (con sset = sample = muestra) y sus programas de las listas 0-3.
 * Los Vagi pasan a 0xFFE2B4A0 + desplazamiento (el BD va a 0x050A0). Devuelve 1 si vale. */
static int coopVoBuild(unsigned char* hd)
{
    unsigned char* e;
    unsigned char* m;
    unsigned char* v;
    int nv;
    int np;
    int i;
    int j;
    int s;

    nv = coopHdNum(coopHdChunk(hd, 3));
    np = coopHdNum(coopHdChunk(hd, 0));

    if ((nv > COOP_VO_N) || (coopHdNum(coopHdChunk(hd, 2)) != nv) || (coopHdNum(coopHdChunk(hd, 1)) != nv))
    {
        return 0;
    }

    if (coopRd32(&hd[0x20]) > COOP_VO_MAX)
    {
        return 0;
    }

    for (i = 0; i < nv; i++)
    {
        e = coopHdEnt(coopHdChunk(hd, 1), i);
        m = coopHdEnt(coopHdChunk(hd, 2), i);
        v = coopHdEnt(coopHdChunk(hd, 3), i);

        if ((e == NULL) || (m == NULL) || (v == NULL) || (e[3] != 1) || (coopRd16(&e[4]) != i) || (coopRd16(m) != i))
        {
            return 0;
        }

        coopCopy(coop_vo_vag[i], v, COOP_HD_VAGI);
        coopWr32(coop_vo_vag[i], COOP_VO_OFF + coopRd32(v));

        coopCopy(coop_vo_smp[i], m, COOP_HD_SMPL);
        coopCopy(coop_vo_ss[i], e, COOP_HD_SSET);
    }

    for (i = 0; i < COOP_VO_PROG; i++)
    {
        coop_vo_psz[i] = 0;

        e = (i < np) ? coopHdEnt(coopHdChunk(hd, 0), i) : NULL;

        if (e == NULL)
        {
            continue;
        }

        s = coopRd32(e) + (e[4] * e[5]);

        if ((e[4] == 0) || (s > COOP_SE_PSZ))
        {
            continue;
        }

        for (j = 0; j < e[4]; j++)
        {
            if (coopRd16(&e[coopRd32(e) + (j * e[5])]) >= nv)
            {
                break;
            }
        }

        if (j < e[4])
        {
            continue;
        }

        coopCopy(coop_vo_prog[i], e, s);

        coop_vo_psz[i] = s;
    }

    coopCopy(coop_vo_head, hd, 0x50);

    coop_vo_n = nv;
    coop_vo_bdsz = coopRd32(&hd[0x20]);

    return 1;
}

/* Entrada k (0 Vagi, 1 Smpl, 2 Sset) número i de las añadidas tras las de P1: armas de P2 (na),
 * la entrada falsa y la voz de P2. */
static unsigned char* coopSeExtra(int k, int i, int na)
{
    if (i < na)
    {
        return (k == 0) ? coop_p2_vag[i] : (k == 1) ? coop_p2_smp[i] : coop_p2_ss[i];
    }

    if (i == na)
    {
        return (k == 0) ? coop_dum_vag : (k == 1) ? coop_dum_smp : coop_dum_ss;
    }

    i -= na + 1;

    return (k == 0) ? coop_vo_vag[i] : (k == 1) ? coop_vo_smp[i] : coop_vo_ss[i];
}

/* HD del banco 4: el de voz de P1 (si ya se cargó), los programas de armas de P2 en 32 + lista y la voz
 * de P2 en las listas 4-7 (si está cargada). Devuelve su tamaño o 0; coop_mrg_vo dice si lleva la voz de P2. */
static int coopSeMerge(void)
{
    unsigned char* hd;
    unsigned char* c;
    unsigned char* e;
    unsigned int end;
    int nv;
    int nm;
    int ns;
    int np;
    int na;
    int nx;
    int vo;
    int n;
    int i;
    int j;
    int o;
    int pos;
    int vpos;
    int mpos;
    int spos;
    int ppos;

    hd = coop_se_hd;

    coop_mrg_vo = 0;

    na = coop_p2_n;
    vo = coop_vo_ok;

    nv = nm = ns = np = 0;

    if (coop_core_ok != 0)
    {
        nv = coopHdNum(coopHdChunk(coop_core_hd, 3));
        nm = coopHdNum(coopHdChunk(coop_core_hd, 2));
        ns = coopHdNum(coopHdChunk(coop_core_hd, 1));
        np = coopHdNum(coopHdChunk(coop_core_hd, 0));

        if (np > COOP_VO_LIST)
        {
            vo = 0;
        }
    }

    nx = na + ((vo != 0) ? coop_vo_n + 1 : 0);

    if ((np > COOP_SE_PROG) || ((nv + nx) > (COOP_SE_PROG * 2)) || ((nm + nx) > (COOP_SE_PROG * 2)) || ((ns + nx) > (COOP_SE_PROG * 2)))
    {
        return 0;
    }

    coopCopy(hd, (coop_core_ok != 0) ? coop_core_hd : (na != 0) ? coop_p2_head : coop_vo_head, 0x50);

    /* Entrada falsa: empieza donde acaba lo anterior, para que su duración sea la buena. */
    if (vo != 0)
    {
        end = (na != 0) ? COOP_SE_BASE + coop_p2_bdsz : (coop_core_ok != 0) ? coopRd32(&coop_core_hd[0x20]) : 0;

        coopCopy(coop_dum_vag, coop_vo_vag[0], COOP_HD_VAGI);
        coopWr32(coop_dum_vag, end);

        coopCopy(coop_dum_smp, coop_vo_smp[0], COOP_HD_SMPL);
        coopCopy(coop_dum_ss, coop_vo_ss[0], COOP_HD_SSET);
    }

    /* Vagi */
    for (i = 0; i < nv; i++)
    {
        coop_ent[i] = coopHdEnt(coopHdChunk(coop_core_hd, 3), i);
        coop_esz[i] = COOP_HD_VAGI;
    }

    for (i = 0; i < nx; i++)
    {
        coop_ent[nv + i] = coopSeExtra(0, i, na);
        coop_esz[nv + i] = COOP_HD_VAGI;
    }

    vpos = 0x50;
    mpos = coopSeChunk(vpos, "IECSigaV", nv + nx);

    if (mpos < 0)
    {
        return 0;
    }

    /* Smpl */
    for (i = 0; i < nm; i++)
    {
        coop_ent[i] = coopHdEnt(coopHdChunk(coop_core_hd, 2), i);
        coop_esz[i] = COOP_HD_SMPL;
    }

    for (i = 0; i < nx; i++)
    {
        coop_ent[nm + i] = coopSeExtra(1, i, na);
        coop_esz[nm + i] = COOP_HD_SMPL;
    }

    spos = coopSeChunk(mpos, "IECSlpmS", nm + nx);

    if (spos < 0)
    {
        return 0;
    }

    for (i = 0; i < nx; i++)
    {
        e = coopHdEnt(&hd[mpos], nm + i);

        coopWr16(e, nv + i);
    }

    /* Sset */
    for (i = 0; i < ns; i++)
    {
        coop_ent[i] = coopHdEnt(coopHdChunk(coop_core_hd, 1), i);
        coop_esz[i] = (coop_ent[i] != NULL) ? 4 + (coop_ent[i][3] * 2) : 0;
    }

    for (i = 0; i < nx; i++)
    {
        coop_ent[ns + i] = coopSeExtra(2, i, na);
        coop_esz[ns + i] = COOP_HD_SSET;
    }

    ppos = coopSeChunk(spos, "IECStesS", ns + nx);

    if (ppos < 0)
    {
        return 0;
    }

    for (i = 0; i < nx; i++)
    {
        e = coopHdEnt(&hd[spos], ns + i);

        coopWr16(&e[4], nm + i);
    }

    /* Prog: los de voz de P1 en su sitio, los de voz de P2 en 4 + lista y los de armas de P2 en 32 + lista. */
    n = COOP_SE_PROG * 2;

    for (i = 0; i < n; i++)
    {
        coop_ent[i] = NULL;
        coop_esz[i] = 0;

        if (i < np)
        {
            coop_ent[i] = coopHdEnt(coopHdChunk(coop_core_hd, 0), i);

            if (coop_ent[i] != NULL)
            {
                coop_esz[i] = coopRd32(coop_ent[i]) + (coop_ent[i][4] * coop_ent[i][5]);
            }
        }
        else if ((vo != 0) && (i >= COOP_VO_LIST) && (i < (COOP_VO_LIST + COOP_VO_PROG)) && (coop_vo_psz[i - COOP_VO_LIST] != 0))
        {
            coop_ent[i] = coop_vo_prog[i - COOP_VO_LIST];
            coop_esz[i] = coop_vo_psz[i - COOP_VO_LIST];
        }
        else if ((i >= COOP_SE_PROG) && (coop_p2_psz[i - COOP_SE_PROG] != 0))
        {
            coop_ent[i] = coop_p2_prog[i - COOP_SE_PROG];
            coop_esz[i] = coop_p2_psz[i - COOP_SE_PROG];
        }
    }

    pos = coopSeChunk(ppos, "IECSgorP", n);

    if (pos < 0)
    {
        return 0;
    }

    c = &hd[ppos];

    for (i = np; i < n; i++)
    {
        e = coopHdEnt(c, i);

        if (e == NULL)
        {
            continue;
        }

        o = (i >= COOP_SE_PROG) ? ns : ns + na + 1;

        for (j = 0; j < e[4]; j++)
        {
            coopWr16(&e[coopRd32(e) + (j * e[5])], o + coopRd16(&e[coopRd32(e) + (j * e[5])]));
        }
    }

    /* Head: tamaños y posiciones. El tamaño del BD lo usa el driver para la duración de la última muestra. */
    if (vo != 0)
    {
        end = COOP_VO_OFF + coop_vo_bdsz;
    }
    else if (na != 0)
    {
        end = COOP_SE_BASE + coop_p2_bdsz;
    }
    else
    {
        end = coopRd32(&hd[0x20]);
    }

    coopWr32(&hd[0x1C], pos);
    coopWr32(&hd[0x20], end);
    coopWr32(&hd[0x24], ppos);
    coopWr32(&hd[0x28], spos);
    coopWr32(&hd[0x2C], mpos);
    coopWr32(&hd[0x30], vpos);

    while ((pos & 63) != 0)
    {
        if (pos >= COOP_SE_HD)
        {
            return 0;
        }

        hd[pos++] = 0;
    }

    coop_mrg_vo = vo;

    return pos;
}

/* G20 (LoadSoundPackFile, case 2, antes de SetSoundData). En la interrupción de VSync. */
void coopSePack(SPQ_HEADER* h, unsigned char* buf)
{
    unsigned int* blk;
    unsigned char* hd;
    unsigned char* bd;
    int size;

    coop_bd_off = 0;

    if (coop_se_swap != 0)
    {
        SE_BANK[COOP_SE_BANK] = coop_se_bank_bak;

        coop_se_swap = 0;
    }

    if (h->Type != 2)
    {
        return;
    }

    blk = (unsigned int*)&buf[h->Offset];

    hd = (unsigned char*)blk + blk[0];
    bd = (unsigned char*)blk + blk[2];

    if ((SpqFileReadRequestFlag == COOP_SE_REQ) && (h->BankNo == 1))
    {
        /* Banco de armas de P2: al banco 4, con el BD en el hueco de la SPU2. */
        coopSeBuildP2(hd, bd, blk[3]);

        size = coopSeMerge();

        if (size != 0)
        {
            blk[0] = (unsigned int)coop_se_hd - (unsigned int)blk;
            blk[1] = size;

            coop_vo_in = coop_mrg_vo;
        }
        else
        {
            /* No cabe: el banco 4 recibe el HD de voz tal cual (o, sin él, el del arma, que no se usa).
             * Nunca se deja seguir hacia el banco 1, que es el de P1. */
            coop_p2_n = 0;
            coop_vo_in = 0;

            if (coop_core_ok != 0)
            {
                coopCopy(coop_se_hd, coop_core_hd, coop_core_sz);

                blk[0] = (unsigned int)coop_se_hd - (unsigned int)blk;
                blk[1] = coop_core_sz;
            }
        }

        blk[3] = (coop_p2_bdsz != 0) ? coop_p2_bdsz : 64;

        h->BankNo = COOP_SE_BANK;

        coop_se_bank_bak = SE_BANK[COOP_SE_BANK];
        SE_BANK[COOP_SE_BANK] = COOP_SE_PORT;

        coop_se_swap = 1;
        coop_se_built = (coop_p2_n != 0) ? 1 : 0;
    }
    else if ((SpqFileReadRequestFlag == COOP_VO_REQ) && (h->BankNo == COOP_SE_BANK))
    {
        /* Voz de P2: el BD va al hueco bajo (G28) y el HD se fusiona con el de P1. Su BD nunca va a la
         * zona del puerto 7; si no vale, solo se suben 64 bytes al hueco. */
        coop_vo_id = SpqKeyCode & 0xF;
        coop_vo_ok = (blk[3] <= COOP_VO_MAX) ? coopVoBuild(hd) : 0;

        size = coopSeMerge();

        if ((size == 0) && (coop_vo_ok != 0))
        {
            coop_vo_ok = 0;

            size = coopSeMerge();
        }

        if (size != 0)
        {
            blk[0] = (unsigned int)coop_se_hd - (unsigned int)blk;
            blk[1] = size;

            coop_vo_in = coop_mrg_vo;
        }
        else
        {
            coop_vo_in = 0;

            if (coop_core_ok != 0)
            {
                coopCopy(coop_se_hd, coop_core_hd, coop_core_sz);

                blk[0] = (unsigned int)coop_se_hd - (unsigned int)blk;
                blk[1] = coop_core_sz;
            }
        }

        if (coop_vo_ok == 0)
        {
            blk[3] = 64;
        }

        coop_bd_off = COOP_VO_TRANS;
        coop_vo_built = coop_vo_ok;
    }
    else if (h->BankNo == COOP_SE_BANK)
    {
        /* Voz de P1: se guarda su HD y, si P2 tiene sonidos o voz, se le añaden. */
        if (SpqFileReadRequestFlag == 4)
        {
            coop_vo_p1 = SpqKeyCode & 0xF;
        }

        coop_vo_in = 0;

        if (blk[1] > COOP_SE_CORE)
        {
            coop_core_ok = 0;
            return;
        }

        coopCopy(coop_core_hd, hd, blk[1]);

        coop_core_sz = blk[1];
        coop_core_ok = 1;

        if ((coop_p2_n != 0) || (coop_vo_ok != 0))
        {
            size = coopSeMerge();

            if (size != 0)
            {
                blk[0] = (unsigned int)coop_se_hd - (unsigned int)blk;
                blk[1] = size;

                coop_vo_in = coop_mrg_vo;
            }
        }
    }
}

/* G28 (sdBankDownload, banco de SE, tras iop_trans_offset = 0): desplazamiento inicial del BD.
 * Vale 0x2B4A0 justo después de que G20 prepare la voz de P2 (0x1D9C00 + 0x2B4A0 = 0x2050A0, que la
 * SPU2 lleva a 0x050A0) y 0 en cualquier otra carga. */
unsigned int coopSeBdOffset(void)
{
    unsigned int o;

    o = coop_bd_off;

    coop_bd_off = 0;

    return o;
}

/* G21 (CallPlayerWeaponSeEx): durante el update de P2, sus sonidos van al banco 4. */
int coopWeaponSeNo(int se)
{
    int l;

    l = se & 0xFF;

    if ((plp != &ply2) || (coop_se_state != 2) || (l >= COOP_SE_PROG) || (coop_p2_psz[l] == 0))
    {
        return se;
    }

    return (se & 0xFFFF0000) | (COOP_SE_BANK << 8) | (COOP_SE_PROG + l);
}

/* G23 (CallPlayerVoice): con plp = P2 (su update o un enemigo que va a por él), la voz del banco 4
 * (listas 0-3) va a la de P2 (listas 4-7) si es otro personaje y su voz está en el HD del IOP. */
int coopVoiceSeNo(int se)
{
    int l;

    l = se & 0xFF;

    if ((plp != &ply2) || (coop_vo_state != 2) || (coop_vo_in == 0) || (coop_vo_id == coop_vo_p1))
    {
        return se;
    }

    if ((((se >> 8) & 0xF) != COOP_SE_BANK) || (l >= COOP_VO_PROG) || (coop_vo_psz[l] == 0))
    {
        return se;
    }

    return (se & ~0xFF) | (COOP_VO_LIST + l);
}

/* Pide el banco ARMS_xxx de P2 (snd_wpno; -1 = ninguno). */
void coopSeLoad(int snd)
{
    coop_se_want = snd;
}

/* Pide la voz CORE_<id> de P2 (-1 = ninguna). Se vuelve a subir aunque sea la misma: no se sabe
 * si el hueco bajo de la SPU2 sigue intacto después de una carga completa. */
void coopVoiceLoad(int id)
{
    coop_vo_want = ((id >= 0) && (id <= 3)) ? id : -1;
    coop_vo_cur = -2;
}

/* Carga del banco de armas de P2. Devuelve 1 cuando ha terminado (o no hay nada que hacer). */
static int coopSeStepWpn(void)
{
    if (coop_se_state == 1)
    {
        if (SpqFileReadRequestFlag == COOP_SE_REQ)
        {
            return 0;
        }

        if (coop_se_swap != 0)
        {
            SE_BANK[COOP_SE_BANK] = coop_se_bank_bak;

            coop_se_swap = 0;
        }

        coop_se_state = (coop_se_built != 0) ? 2 : 0;

        printf("[COOP] sonidos de P2: banco %d, %d muestras, %d B\n", coop_se_cur, coop_p2_n, coop_p2_bdsz);
        return 1;
    }

    if (coop_se_want == coop_se_cur)
    {
        return 1;
    }

    if (coop_se_want < 0)
    {
        coop_se_cur = coop_se_want;
        coop_se_state = 0;
        return 1;
    }

    if (SpqFileReadRequestFlag != 0)
    {
        return 0;
    }

    coop_se_cur = coop_se_want;
    coop_se_built = 0;
    coop_se_state = 1;

    SpqKeyCode = coop_se_want | 0x4000;
    SpqFileReadRequestFlag = COOP_SE_REQ;

    return 0;
}

/* Carga de la voz de P2. Devuelve 1 cuando ha terminado (o no hay nada que hacer). */
static int coopSeStepVoice(void)
{
    if (coop_vo_state == 1)
    {
        if (SpqFileReadRequestFlag == COOP_VO_REQ)
        {
            return 0;
        }

        coop_vo_state = (coop_vo_built != 0) ? 2 : 0;

        printf("[COOP] voz de P2: CORE_%03d, %d muestras, %d B (voz de P1: %d)\n", coop_vo_cur, (coop_vo_built != 0) ? coop_vo_n : 0, coop_vo_bdsz, coop_vo_p1);
        return 1;
    }

    if (coop_vo_want == coop_vo_cur)
    {
        return 1;
    }

    if (coop_vo_want < 0)
    {
        coop_vo_cur = coop_vo_want;
        coop_vo_state = 0;
        return 1;
    }

    if (SpqFileReadRequestFlag != 0)
    {
        return 0;
    }

    coop_vo_cur = coop_vo_want;
    coop_vo_built = 0;
    coop_vo_state = 1;

    SpqKeyCode = coop_vo_want | 0xFFF0;
    SpqFileReadRequestFlag = COOP_VO_REQ;

    return 0;
}

/* Lleva las cargas pedidas con coopSeLoad y coopVoiceLoad (primero el arma). Devuelve 1 al terminar las dos. */
int coopSeStep(void)
{
    if (coopSeStepWpn() == 0)
    {
        return 0;
    }

    return coopSeStepVoice();
}

#endif
