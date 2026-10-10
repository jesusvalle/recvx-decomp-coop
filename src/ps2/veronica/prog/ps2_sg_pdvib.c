#include "../../../ps2/veronica/prog/ps2_sg_pdvib.h"  
#include "../../../ps2/veronica/prog/main.h"

#ifdef COOP
#include "../../../ps2/veronica/prog/coop.h"

Sint32 coopVibIsReady2(void);
Sint32 coopVibStart2(const PDS_VIBPARAM* param);
#endif

PAD_ACT Pad_act[20];
unsigned char Ps2_pad_motor[6];
unsigned char Pad_vibe_call_num;

// 100% matching! 
Sint32 pdVibMxIsReady(Uint32 port)
{ 
    unsigned int pad_info; 
    unsigned int pad_stat; 
    unsigned char i; 

#ifdef COOP
    if (port == 8)
    {
        return coopVibIsReady2();
    }
#endif

    pad_stat = scePadGetState(0, 0);

    if ((pad_stat != scePadStateStable) && (pad_stat != scePadStateFindCTP1))
    {
        return 0; 
    }

    i = 6;
    
    while (i--) 
    { 
        Ps2_pad_motor[i] = 0; 
    }

    pad_info = scePadInfoMode(0, 0, InfoModeCurExID, 0);

    if ((pad_info != 7) && (pad_info != 4))
    {
        return 0;
    }

    return 1; 
}

// 100% matching! 
Sint32 pdVibMxSetStopTime(Uint32 port, Uint32 time)
{
    return 0;
}

// 100% matching! 
Sint32 pdVibMxStart(Uint32 port, const PDS_VIBPARAM* param)
{
    unsigned char cnt;
    PAD_ACT* pPact;

#ifdef COOP
    if (port == 8)
    {
        return coopVibStart2(param);
    }
#endif

    pPact = Pad_act;
    
    cnt = 0;
    
    while (pPact->be_flag != 0)
    {
        cnt++;
        pPact++;
        
        if (cnt >= 20) 
        {
            return -1;
        }
    }

    pPact->be_flag = 3;
    
    if (param->power > 85) 
    {
        pPact->level = pPact->start = 255;
    } 
    else
    {
        pPact->level = pPact->start = param->power + 170;
    }

    if (param->flag == 1) 
    {
        pPact->time = 9;
    } 
    else if (param->flag == 8) 
    {
        pPact->time = 6;
    }
    else 
    {
        pPact->time = 4;
    }
}

// 100% matching! 
Sint32 pdVibMxStop(Uint32 port)
{
#ifdef COOP
    if (port == 8)
    {
        coopVibStop2();
        return 0;
    }

    coopVibStop2();
#endif

    Ps2_pad_act_all_stop();
    
    return 0;
}

// 100% matching! 
void Ps2_pad_actuater()
{
    PAD_ACT* pPact;
    unsigned int cnt;

    pPact = Pad_act;
    
    Ps2_pad_motor[0] = 0;
    Ps2_pad_motor[1] = 0;
    
    for (cnt = 0; cnt < 19; cnt++) 
    {
        if ((pPact->be_flag & 0x3)) 
        {
            if (pPact->delay > 0) 
            {
                pPact->delay--;
            }
            else if (pPact->time > 0) 
            {
                Ps2_pad_act_start(pPact);
                
                pPact->time--;
            } 
            else
            {
                Ps2_pad_act_stop(pPact);
            }
        }
        
        pPact++;
    } 
    
    Pad_vibe_call_num = 0;
    
    scePadSetActDirect(0, 0, Ps2_pad_motor);
}

// 100% matching! 
static void Ps2_pad_act_start(PAD_ACT* pPact)
{
    unsigned char temp; // not from the debugging symbols

    if ((pPact->be_flag & 0x2)) 
    {
        if (pPact->f_add != 0) 
        {
            pPact->level = (((short)pPact->start * 16) - (short)pPact->f_add) / 16;
        }
        
        temp = pPact->level;
        
        if (Ps2_pad_motor[1] < temp)
        {
            Ps2_pad_motor[1] = temp;
        }
        
        if (pPact->f_level != 0) 
        {
            pPact->f_add += pPact->f_level;
        }
    }
    
    if (pPact->f_level != 0) 
    {
        pPact->f_add += pPact->f_level;
    }
}

// 100% matching! 
static void Ps2_pad_act_stop(PAD_ACT* pPact)
{
    pPact->be_flag = 0;
}

// 100% matching! 
static void Ps2_pad_act_all_stop()
{
    PAD_ACT* pPact; 
    int i; 
    
    pPact = Pad_act;
    
    i = 19;
    
    do
    {
        pPact->be_flag = 0;
        
        pPact++; 
    } while (i--);
}

#ifdef COOP

/* Vibración del mando 2 (P2): copia de la de P1 sobre su propia tabla, enviada al puerto 1. */
static PAD_ACT Pad_act2[20];
static unsigned char Ps2_pad_motor2[6] __attribute__((aligned(16)));
static int coop_vib_on;

Sint32 coopVibIsReady2(void)
{
    unsigned int pad_info;
    unsigned int pad_stat;
    unsigned char i;

    pad_stat = scePadGetState(1, 0);

    if ((pad_stat != scePadStateStable) && (pad_stat != scePadStateFindCTP1))
    {
        return 0;
    }

    i = 6;

    while (i--)
    {
        Ps2_pad_motor2[i] = 0;
    }

    pad_info = scePadInfoMode(1, 0, InfoModeCurExID, 0);

    if ((pad_info != 7) && (pad_info != 4))
    {
        return 0;
    }

    return 1;
}

Sint32 coopVibStart2(const PDS_VIBPARAM* param)
{
    unsigned char cnt;
    PAD_ACT* pPact;

    pPact = Pad_act2;
    cnt = 0;

    while (pPact->be_flag != 0)
    {
        cnt++;
        pPact++;

        if (cnt >= 20)
        {
            return -1;
        }
    }

    pPact->be_flag = 3;

    if (param->power > 85)
    {
        pPact->level = pPact->start = 255;
    }
    else
    {
        pPact->level = pPact->start = param->power + 170;
    }

    if (param->flag == 1)
    {
        pPact->time = 9;
    }
    else if (param->flag == 8)
    {
        pPact->time = 6;
    }
    else
    {
        pPact->time = 4;
    }

    return 0;
}

void coopVibStop2(void)
{
    int i;

    for (i = 0; i < 20; i++)
    {
        Pad_act2[i].be_flag = 0;
    }

    if (coop_vib_on != 0)
    {
        Ps2_pad_motor2[0] = 0;
        Ps2_pad_motor2[1] = 0;

        scePadSetActDirect(1, 0, Ps2_pad_motor2);

        coop_vib_on = 0;
    }
}

static void coopPadActStart2(PAD_ACT* pPact)
{
    unsigned char temp;

    if ((pPact->be_flag & 0x2))
    {
        if (pPact->f_add != 0)
        {
            pPact->level = (((short)pPact->start * 16) - (short)pPact->f_add) / 16;
        }

        temp = pPact->level;

        if (Ps2_pad_motor2[1] < temp)
        {
            Ps2_pad_motor2[1] = temp;
        }

        if (pPact->f_level != 0)
        {
            pPact->f_add += pPact->f_level;
        }
    }

    if (pPact->f_level != 0)
    {
        pPact->f_add += pPact->f_level;
    }
}

void coopPadActuater2(void)
{
    PAD_ACT* pPact;
    unsigned int cnt;

    pPact = Pad_act2;

    Ps2_pad_motor2[0] = 0;
    Ps2_pad_motor2[1] = 0;

    for (cnt = 0; cnt < 19; cnt++)
    {
        if ((pPact->be_flag & 0x3))
        {
            if (pPact->delay > 0)
            {
                pPact->delay--;
            }
            else if (pPact->time > 0)
            {
                coopPadActStart2(pPact);
                pPact->time--;
            }
            else
            {
                pPact->be_flag = 0;
            }
        }

        pPact++;
    }

    coop_vib_on = (Ps2_pad_motor2[1] != 0) ? 1 : 0;

    scePadSetActDirect(1, 0, Ps2_pad_motor2);
}

#endif
