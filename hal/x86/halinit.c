/****************************************************************
        Cosmos HAL全局初始化文件halinit.c
*****************************************************************
                彭东
****************************************************************/

#include "cosmostypes.h"
#include "cosmosmctrl.h"


void init_hal()
{

    init_halplaltform();
    kprint("1.Call init_halplaltform()\n");
    kprint("    1.1.Call init_machbstart()\n");
    kprint("    1.2.Call init_bdvideo()\n");

    kprint("2.Call move_img2maxpadr(&kmachbsp)\n");
    move_img2maxpadr(&kmachbsp);

    kprint("3.Call init_halmm()\n");
    init_halmm();

    kprint("4.Call init interrupt()\n");
    init_halintupt();
    return;
}
