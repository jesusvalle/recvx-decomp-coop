#ifndef _COOPCAM_H_
#define _COOPCAM_H_

#ifdef COOP_SPLIT

#include "types.h"

/* sys->itm[275] (COOP_ITM + 19): 0 = dos cámaras, 1 = una. Viaja con la partida. */
#define COOP_CAM_OPT 275
/* Máximo de rom->lgt_n en las 205 salas: 44 (RM_0020, RM_0021). */
#define COOP_LGT_MAX 48

void coopCamRoomStart(void);
void coopUpdateCamera2(void);
int coopSplitDraw(void);
void coopDrawMeter(void);
void coopFrameStat(unsigned int vc);
int coopSplitShowP2(void);
void coopCamP2Begin(void);
void coopCamP2End(void);

#endif

#endif
