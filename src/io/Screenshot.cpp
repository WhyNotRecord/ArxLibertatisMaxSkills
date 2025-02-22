/*
 * Copyright 2011-2021 Arx Libertatis Team (see the AUTHORS file)
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
// Code: Cyril Meynier
//
// Copyright (c) 1999-2000 ARKANE Studios SA. All rights reserved

#include "io/Screenshot.h"

#include <cstdio>
#include <string>
#include <sstream>
#include <utility>

#include "graphics/Renderer.h"
#include "graphics/image/Image.h"
#include "io/fs/Filesystem.h"

static SnapShot * pSnapShot;

SnapShot::SnapShot(fs::path name)
	: m_basePath(std::move(name))
{ }

fs::path SnapShot::getNextFilePath() {
	int num = 0;

	fs::path file;

	do {
		std::ostringstream oss;
		oss << m_basePath.filename() << '_' << num << ".bmp";

		file = m_basePath.parent() / oss.str();

		num++;
	} while(fs::exists(file));

	return file;
}

bool SnapShot::GetSnapShot() {
	
	Image image;
	
	if(!GRenderer->getSnapshot(image)) {
		return false;
	}
	
	fs::path file = getNextFilePath();

	return image.save(file);
}

void InitSnapShot(fs::path name) {
	FreeSnapShot();
	pSnapShot = new SnapShot(std::move(name));
}

void GetSnapShot() {
	if(pSnapShot) {
		pSnapShot->GetSnapShot();
	}
}

void FreeSnapShot() {
	delete pSnapShot, pSnapShot = nullptr;
}
