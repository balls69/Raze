//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef savegame_h_
#define savegame_h_

#include "game.h"

BEGIN_DUKE_NS


#ifdef LUNATIC
# define SV_MAJOR_VER 2
#else
# define SV_MAJOR_VER 1
#endif
#define SV_MINOR_VER 7

#pragma pack(push,1)
typedef struct
{
    char headerstr[11];
    uint8_t majorver, minorver, ptrsize;
    uint16_t bytever;
    // 16 bytes

    uint32_t userbytever;
    uint32_t scriptcrc;

    uint8_t recdiffsp;
    // 4 bytes

    int32_t reccnt, snapsiz;
    // 8 bytes

    uint8_t numplayers, volnum, levnum, skill;
    // 286 bytes

    uint8_t getPtrSize() const { return ptrsize; }
} savehead_t;
#pragma pack(pop)

extern int32_t g_fakeSaveID;
extern bool g_saveRequested;


int32_t sv_updatestate(int32_t frominit);
int32_t sv_readdiff(FileReader& fil);
uint32_t sv_writediff(FileWriter *fil);
int32_t sv_loadheader(FileReader &fil, int32_t spot, savehead_t *h);
int32_t sv_loadsnapshot(FileReader &fil, int32_t spot, savehead_t *h);
int32_t sv_saveandmakesnapshot(FileWriter &fil, int8_t spot);
void sv_freemem();

#ifdef YAX_ENABLE
extern void sv_postyaxload(void);
#endif

enum
{
    P2I_BACK_BIT = 1,
    P2I_ONLYNON0_BIT = 2,

    P2I_FWD = 0,
    P2I_BACK = 1,

    P2I_FWD_NON0 = 0+2,
    P2I_BACK_NON0 = 1+2,
};
void G_Util_PtrToIdx(void *ptr, int32_t count, const void *base, int32_t mode);
void G_Util_PtrToIdx2(void *ptr, int32_t count, size_t stride, const void *base, int32_t mode);


END_DUKE_NS

#endif
