#ifndef _COOP_H_
#define _COOP_H_

#ifdef COOP

#include "types.h"

#define COOP_P2_HP 160
#define COOP_P2_PAD_MASK 0x5FF
#define COOP_MODEL_POOL_SIZE 65536
#define COOP_HAIR_EXP0_SIZE 0x4000
#define COOP_HAIR_EXP3_SIZE 0x1000
#define COOP_LMM_SIZE 32768
#define COOP_PXLCONV_SIZE 327680

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

const PDS_PERIPHERAL* coopGetPeripheral2(void);
void coopSetPad2(void);
void coopInitMemory(void);
int coopLoadPlayer2(void);
void coopCloneWeapon(void);
void coopRoomStart(void);
void coopControlPlayer2(void);
void coopDrawPlayer2(void);
void coopVibStop2(void);
void coopPadActuater2(void);

#endif

#endif
