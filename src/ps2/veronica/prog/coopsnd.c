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
 * valor propio. LoadSoundPackFile corre en la interrupción de VSync: coopSePack no imprime. */

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

/* HD del banco 4: el de voz (si ya se cargó) más los programas de P2 en 32 + lista. Devuelve su tamaño o 0. */
static int coopSeMerge(void)
{
    unsigned char* hd;
    unsigned char* c;
    unsigned char* e;
    int nv;
    int nm;
    int ns;
    int np;
    int n;
    int i;
    int j;
    int pos;
    int vpos;
    int mpos;
    int spos;
    int ppos;

    hd = coop_se_hd;

    coopCopy(hd, (coop_core_ok != 0) ? coop_core_hd : coop_p2_head, 0x50);

    nv = nm = ns = np = 0;

    if (coop_core_ok != 0)
    {
        nv = coopHdNum(coopHdChunk(coop_core_hd, 3));
        nm = coopHdNum(coopHdChunk(coop_core_hd, 2));
        ns = coopHdNum(coopHdChunk(coop_core_hd, 1));
        np = coopHdNum(coopHdChunk(coop_core_hd, 0));

        if ((np > COOP_SE_PROG) || ((nv + coop_p2_n) > (COOP_SE_PROG * 2)) || ((nm + coop_p2_n) > (COOP_SE_PROG * 2)) || ((ns + coop_p2_n) > (COOP_SE_PROG * 2)))
        {
            return 0;
        }
    }

    /* Vagi */
    for (i = 0; i < nv; i++)
    {
        coop_ent[i] = coopHdEnt(coopHdChunk(coop_core_hd, 3), i);
        coop_esz[i] = COOP_HD_VAGI;
    }

    for (i = 0; i < coop_p2_n; i++)
    {
        coop_ent[nv + i] = coop_p2_vag[i];
        coop_esz[nv + i] = COOP_HD_VAGI;
    }

    vpos = 0x50;
    mpos = coopSeChunk(vpos, "IECSigaV", nv + coop_p2_n);

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

    for (i = 0; i < coop_p2_n; i++)
    {
        coop_ent[nm + i] = coop_p2_smp[i];
        coop_esz[nm + i] = COOP_HD_SMPL;
    }

    spos = coopSeChunk(mpos, "IECSlpmS", nm + coop_p2_n);

    if (spos < 0)
    {
        return 0;
    }

    for (i = 0; i < coop_p2_n; i++)
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

    for (i = 0; i < coop_p2_n; i++)
    {
        coop_ent[ns + i] = coop_p2_ss[i];
        coop_esz[ns + i] = COOP_HD_SSET;
    }

    ppos = coopSeChunk(spos, "IECStesS", ns + coop_p2_n);

    if (ppos < 0)
    {
        return 0;
    }

    for (i = 0; i < coop_p2_n; i++)
    {
        e = coopHdEnt(&hd[spos], ns + i);

        coopWr16(&e[4], nm + i);
    }

    /* Prog: los de voz en su sitio y los de P2 en 32 + lista. */
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

    for (i = COOP_SE_PROG; i < n; i++)
    {
        e = coopHdEnt(c, i);

        if (e != NULL)
        {
            for (j = 0; j < e[4]; j++)
            {
                coopWr16(&e[coopRd32(e) + (j * e[5])], ns + coopRd16(&e[coopRd32(e) + (j * e[5])]));
            }
        }
    }

    /* Head: tamaños y posiciones. El tamaño del BD lo usa el driver para la duración de la última muestra. */
    coopWr32(&hd[0x1C], pos);
    coopWr32(&hd[0x20], COOP_SE_BASE + coop_p2_bdsz);
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

    return pos;
}

/* G20 (LoadSoundPackFile, case 2, antes de SetSoundData). En la interrupción de VSync. */
void coopSePack(SPQ_HEADER* h, unsigned char* buf)
{
    unsigned int* blk;
    unsigned char* hd;
    unsigned char* bd;
    int size;

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
        }
        else
        {
            /* No cabe: el banco 4 recibe el HD de voz tal cual (o, sin él, el del arma, que no se usa).
             * Nunca se deja seguir hacia el banco 1, que es el de P1. */
            coop_p2_n = 0;

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
    else if (h->BankNo == COOP_SE_BANK)
    {
        /* Banco de voz: se guarda su HD y, si P2 tiene sonidos, se le añaden. */
        if (blk[1] > COOP_SE_CORE)
        {
            coop_core_ok = 0;
            return;
        }

        coopCopy(coop_core_hd, hd, blk[1]);

        coop_core_sz = blk[1];
        coop_core_ok = 1;

        if (coop_p2_n != 0)
        {
            size = coopSeMerge();

            if (size != 0)
            {
                blk[0] = (unsigned int)coop_se_hd - (unsigned int)blk;
                blk[1] = size;
            }
        }
    }
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

/* Pide el banco ARMS_xxx de P2 (snd_wpno; -1 = ninguno). */
void coopSeLoad(int snd)
{
    coop_se_want = snd;
}

/* Lleva la carga pedida con coopSeLoad. Devuelve 1 cuando ha terminado (o no hay nada que hacer). */
int coopSeStep(void)
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

#endif
