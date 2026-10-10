#ifndef _COOP_H_
#define _COOP_H_

#ifdef COOP

#include "types.h"

#define COOP_P2_PAD_MASK 0x5FF
#define COOP_MODEL_POOL_SIZE 65536
#define COOP_HAIR_EXP0_SIZE 0x4000
#define COOP_HAIR_EXP3_SIZE 0x1000
#define COOP_LMM_SIZE 32768
#define COOP_PXLCONV_SIZE 327680
#define COOP_ITM 256
#define COOP_MAGIC 0x434F4F50
#define COOP_MN_P2 0x50
#define COOP_MNW_N 512
#define COOP_WMT_SIZE 49152
#define COOP_WMDL_SIZE 49152

typedef struct COOP_PAD
{
    unsigned int on;
    unsigned int oncpy;
    unsigned int ps;
    unsigned int rs;
    unsigned int old;
    unsigned int onb;
    unsigned int psb;
    unsigned int oldb;
    short ax;
    short ay;
    short dx;
    short dy;
    unsigned short ar;
    unsigned short al;
} COOP_PAD;

extern BH_PWORK ply2;

struct SPQ_HEADER;

const PDS_PERIPHERAL* coopGetPeripheral2(void);
void coopSetPad2(void);
void coopInitMemory(void);
int coopLoadPlayer2(void);
void coopSyncBodyMotions(void);
int coopMonitorWeapon2(void);
void coopEff007Mag(O_WRK* op);
void coopPreSubTask(void);
void coopPostSubTask(void);
void coopStatusInit(void);
void coopItemselectBegin(void);
void coopItemselectEnd(void);
int coopItemUseBlocked(S_WORK* st);
void coopEnemyBegin(BH_PWORK* ep);
void coopEnemyEnd(BH_PWORK* ep);
void coopEffectBegin(O_WRK* op);
void coopEffectEnd(O_WRK* op);
void coopCheckBombP2(NJS_POINT3* pos, float ar, int dmax, int dmin);
void coopRoomStart(void);
void coopControlPlayer2(void);
void coopDrawPlayer2(void);
void coopVibStop2(void);
void coopSePack(struct SPQ_HEADER* h, unsigned char* buf);
int coopWeaponSeNo(int se);
void coopSeLoad(int snd);
int coopSeStep(void);
void coopPadActuater2(void);

#endif

#endif
