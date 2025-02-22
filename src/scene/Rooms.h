/*
 * Copyright 2011-2022 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */
/* Based on:
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code').

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/

#ifndef ARX_SCENE_ROOMS_H
#define ARX_SCENE_ROOMS_H

#include <memory>
#include <vector>

#include "graphics/BaseGraphicsTypes.h"
#include "graphics/GraphicsTypes.h"
#include "math/Types.h"
#include "platform/Platform.h"


struct RoomPortal {
	
	Vec3f p[4];
	Plane plane;
	Sphere bounds;
	float minY;
	float maxY;
	
	u32 room0; // facing normal
	u32 room1;
	
	short useportal;
	
	RoomPortal()
		: minY(0.f)
		, maxY(0.f)
		, room0(0)
		, room1(0)
		, useportal(0)
	{
		p[0] = p[1] = p[2] = p[3] = Vec3f(0.f);
	}
	
};

struct EP_DATA {
	
	Vec2s tile;
	short idx;
	
	EP_DATA()
		: tile(0)
		, idx(0)
	{ }
	
};

struct Room {
	
	std::vector<long> portals;
	std::vector<EP_DATA> epdata;
	std::vector<unsigned short> indexBuffer;
	std::unique_ptr<VertexBuffer<SMY_VERTEX>> pVertexBuffer;
	std::vector<TextureContainer *> ppTextureContainer;
	
};

#define MAX_FRUSTRUMS 32

struct EERIE_FRUSTRUM_DATA {
	
	long nb_frustrums;
	EERIE_FRUSTRUM frustrums[MAX_FRUSTRUMS];
	
};

struct PORTAL_ROOM_DRAW {
	
	short count;
	EERIE_FRUSTRUM_DATA frustrum;
	
};

struct RoomData {
	
	std::vector<Room> rooms;
	std::vector<RoomPortal> portals;
	
	std::vector<PORTAL_ROOM_DRAW> visibility;
	std::vector<u32> visibleRooms;
	
};

extern RoomData * g_rooms;

#endif // ARX_SCENE_ROOMS_H
