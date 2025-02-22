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
// Initial Code: Cyril Meynier
//
// Copyright (c) 1999-2001 ARKANE Studios SA. All rights reserved

/*!
 * ARX FTL file loading and saving
 * FTL files contains Optimised/Pre-computed versions of objects for faster loads
 */

#include "graphics/data/FTL.h"

#include <cstdlib>
#include <cstring>

#include "graphics/data/FTLFormat.h"
#include "graphics/data/TextureContainer.h"

#include "io/resource/ResourcePath.h"
#include "io/resource/PakReader.h"
#include "io/Blast.h"
#include "io/log/Logger.h"

#include "platform/Platform.h"

#include "scene/Object.h"

#include "util/String.h"

EERIE_3DOBJ * ARX_FTL_Load(const res::path & file) {
	
	// Creates FTL file name
	res::path filename = (res::path("game") / file).set_ext("ftl");
	
	// Checks for FTL file existence
	PakFile * pf = g_resources->getFile(filename);
	if(!pf) {
		return nullptr;
	}
	
	std::string buffer = pf->read();
	if(buffer.size() < 3) {
		LogError << "Error loading FTL file: " << filename;
		return nullptr;
	}
	
	// Check if we have an uncompressed FTL file
	if(buffer[0] != 'F' || buffer[1] != 'T' || buffer[2] != 'L') {
		buffer = blast(buffer);
		if(buffer.size() < 3) {
			LogError << "Error decompressing FTL file: " << filename;
			return nullptr;
		}
	}
	
	const char * dat = buffer.data();
	size_t pos = 0; // The position within the data
	
	// Pointer to Primary Header
	const ARX_FTL_PRIMARY_HEADER * afph = reinterpret_cast<const ARX_FTL_PRIMARY_HEADER *>(dat + pos);
	pos += sizeof(ARX_FTL_PRIMARY_HEADER);
	
	// Verify FTL file signature
	if(afph->ident[0] != 'F' || afph->ident[1] != 'T' || afph->ident[2] != 'L') {
		LogError << "Invalid FTL file: " << filename;
		return nullptr;
	}
	
	// Verify FTL file version
	if(afph->version != CURRENT_FTL_VERSION) {
		LogError << "Unexpected ftl version " << afph->version << ", expected "
		         << CURRENT_FTL_VERSION << " in " << filename;
		return nullptr;
	}
	
	// Increases offset by checksum size
	pos += 512;
	
	// Pointer to Secondary Header
	const ARX_FTL_SECONDARY_HEADER * afsh;
	afsh = reinterpret_cast<const ARX_FTL_SECONDARY_HEADER *>(dat + pos);
	if(afsh->offset_3Ddata == -1) {
		LogError << "ARX_FTL_Load: error loading data from " << filename;
		return nullptr;
	}
	pos = afsh->offset_3Ddata;
	
	// Available from here in whole function
	EERIE_3DOBJ * obj = new EERIE_3DOBJ;
	
	const ARX_FTL_3D_DATA_HEADER * af3Ddh;
	af3Ddh = reinterpret_cast<const ARX_FTL_3D_DATA_HEADER *>(dat + pos);
	pos += sizeof(ARX_FTL_3D_DATA_HEADER);
	
	obj->vertexlist.resize(af3Ddh->nb_vertex);
	obj->facelist.resize(af3Ddh->nb_faces);
	obj->texturecontainer.resize(af3Ddh->nb_maps);
	obj->grouplist.resize(af3Ddh->nb_groups);
	obj->actionlist.resize(af3Ddh->nb_action);
	obj->selections.resize(af3Ddh->nb_selections);
	arx_assert(af3Ddh->origin >= 0);
	obj->origin = af3Ddh->origin;
	obj->file = res::path::load(util::loadString(af3Ddh->name));
	
	// Vertices stored as EERIE_OLD_VERTEX, copy in to new one
	for(EERIE_VERTEX & vertex : obj->vertexlist) {
		vertex = *reinterpret_cast<const EERIE_OLD_VERTEX *>(dat + pos);
		pos += sizeof(EERIE_OLD_VERTEX);
	}
	
	obj->vertexWorldPositions.resize(obj->vertexlist.size());
	obj->vertexClipPositions.resize(obj->vertexlist.size());
	obj->vertexColors.resize(obj->vertexlist.size());
	
	// Copy the face data in
	for(EERIE_FACE & face : obj->facelist) {
		
		const EERIE_FACE_FTL * eff = reinterpret_cast<const EERIE_FACE_FTL *>(dat + pos);
		pos += sizeof(EERIE_FACE_FTL);
		
		face.facetype = PolyType::load(eff->facetype);
		face.texid = eff->texid;
		face.transval = eff->transval;
		face.norm = eff->norm.toVec3();
		
		// Copy in all the texture and normals data
		static_assert(IOPOLYVERT_FTL == IOPOLYVERT, "array size mismatch");
		for(size_t kk = 0; kk < IOPOLYVERT_FTL; kk++) {
			face.vid[kk] = eff->vid[kk];
			face.u[kk] = eff->u[kk];
			face.v[kk] = eff->v[kk];
		}
		
	}
	
	// Copy in the texture containers
	for(TextureContainer * & texture : obj->texturecontainer) {
		
		const Texture_Container_FTL * tex;
		tex = reinterpret_cast<const Texture_Container_FTL *>(dat + pos);
		pos += sizeof(Texture_Container_FTL);
		
		if(tex->name[0] == '\0') {
			// Some object files contain textures with empty names
			// Don't bother trying to load them as that will just generate an error message
			texture = nullptr;
		} else {
			// Create the texture and put it in the container list
			res::path name = res::path::load(util::loadString(tex->name)).remove_ext();
			texture = TextureContainer::Load(name, TextureContainer::Level);
		}
		
	}
	
	// Copy in the grouplist data
	for(VertexGroup & group : obj->grouplist) {
		
		const EERIE_GROUPLIST_FTL * rawGroup = reinterpret_cast<const EERIE_GROUPLIST_FTL *>(dat + pos);
		pos += sizeof(EERIE_GROUPLIST_FTL);
		
		group.name = util::toLowercase(util::loadString(rawGroup->name));
		group.origin = rawGroup->origin;
		group.indexes.resize(rawGroup->nb_index);
		group.m_blobShadowSize = rawGroup->siz;
		
	}
	
	// Copy in the group index data
	for(VertexGroup & group : obj->grouplist) {
		if(!group.indexes.empty()) {
			const s32 * begin = reinterpret_cast<const s32 *>(dat + pos);
			pos += sizeof(s32) * group.indexes.size();
			const s32 * end = reinterpret_cast<const s32 *>(dat + pos);
			std::copy(begin, end, group.indexes.begin());
		}
	}
	
	// Copy in the action points data
	for(EERIE_ACTIONLIST & action : obj->actionlist) {
		action = *reinterpret_cast<const EERIE_ACTIONLIST_FTL *>(dat + pos);
		pos += sizeof(EERIE_ACTIONLIST_FTL);
	}
	
	// Copy in the selections data
	for(EERIE_SELECTIONS & selection : obj->selections) {
		
		const EERIE_SELECTIONS_FTL * rawSelection = reinterpret_cast<const EERIE_SELECTIONS_FTL *>(dat + pos);
		pos += sizeof(EERIE_SELECTIONS_FTL);
		
		selection.name = util::toLowercase(util::loadString(rawSelection->name));
		selection.selected.resize(rawSelection->nb_selected);
		
	}
	
	// Copy in the selections selected data
	for(EERIE_SELECTIONS & selection : obj->selections) {
		const s32 * begin = reinterpret_cast<const s32 *>(dat + pos);
		pos += sizeof(s32) * selection.selected.size();
		const s32 * end = reinterpret_cast<const s32 *>(dat + pos);
		std::copy(begin, end, selection.selected.begin());
	}
	
	ARX_UNUSED(pos);
	arx_assert(pos <= buffer.size());
	
	obj->pbox = nullptr; // Reset physics
	
	EERIE_OBJECT_CenterObjectCoordinates(obj);
	EERIE_CreateCedricData(obj);
	// Now we can release our cool FTL file
	EERIE_Object_Precompute_Fast_Access(obj);
	
	LogDebug("ARX_FTL_Load: loaded object " << filename);
	
	return obj;
}
