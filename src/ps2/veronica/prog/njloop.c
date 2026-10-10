#include "../../../ps2/veronica/prog/njloop.h"
#ifdef COOP
#include "../../../ps2/veronica/prog/coop.h"
#endif

// 100% matching!
int main(int argc, char *argv[])
{
#ifdef COOP
    coopFixHeap();
    
#endif
    njUserInit(); 
    
    while (TRUE) 
    { 
        if (njUserMain() < NJD_USER_CONTINUE) 
        { 
            break;
        } 
        
        njWaitVSync();
    } 
    
    njUserExit(); 
} 
