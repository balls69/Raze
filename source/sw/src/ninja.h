//-------------------------------------------------------------------------
/*
Copyright (C) 1997, 2005 - 3D Realms Entertainment

This file is part of Shadow Warrior version 1.2

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Original Source: 1997 - Frank Maddin and Jim Norwood
Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#ifndef NINJA_H
#define NINJA_H

BEGIN_SW_NS

#define NINJA_NORMAL_SPEED          60
#define NINJA_RUN_AWAY_SPEED        130
#define NINJA_FIND_PLAYER_SPEED     100
#define NINJA_CRAWL_SPEED           50
#define NINJA_SWIM_SPEED            50

void InitPlayerSprite(PLAYERp pp);
void InitAllPlayerSprites(void);
void PlayerPanelSetup(void);
void PlayerDeathReset(PLAYERp pp);
void SpawnPlayerUnderSprite(PLAYERp pp);

END_SW_NS

#endif
