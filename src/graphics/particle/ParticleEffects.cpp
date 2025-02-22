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
// Code: Cyril Meynier
//
// Copyright (c) 1999-2000 ARKANE Studios SA. All rights reserved

#include "graphics/particle/ParticleEffects.h"

#include <algorithm>

#include <boost/format.hpp>

#include "core/Application.h"
#include "core/Config.h"
#include "core/Core.h"
#include "core/GameTime.h"

#include "game/Camera.h"
#include "game/Damage.h"
#include "game/EntityManager.h"
#include "game/NPC.h"
#include "game/Player.h"
#include "game/spell/Cheat.h"

#include "gui/Interface.h"

#include "graphics/Draw.h"
#include "graphics/GlobalFog.h"
#include "graphics/GraphicsTypes.h"
#include "graphics/Math.h"
#include "graphics/data/TextureContainer.h"
#include "graphics/effects/PolyBoom.h"
#include "graphics/effects/SpellEffects.h"
#include "graphics/particle/MagicFlare.h"
#include "graphics/particle/ParticleTextures.h"

#include "input/Input.h"

#include "math/GtxFunctions.h"
#include "math/Random.h"
#include "math/RandomVector.h"

#include "platform/profiler/Profiler.h"

#include "physics/Collisions.h"
#include "physics/Physics.h"

#include "scene/GameSound.h"
#include "scene/Interactive.h"
#include "scene/Light.h"
#include "scene/Tiles.h"

static const size_t MAX_PARTICLES = 2200;
static long ParticleCount = 0;
static PARTICLE_DEF g_particles[MAX_PARTICLES];

long NewSpell = 0;

long getParticleCount() {
	return ParticleCount;
}

void createFireParticles(Vec3f pos, int perPos, int delay) {
	for(long nn = 0 ; nn < perPos; nn++) {

		if(Random::getf() >= 0.4f) {
			continue;
		}

		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			break;
		}

		pd->ov = pos;
		pd->move = Vec3f(2.f, 2.f, 2.f) - Vec3f(4.f, 22.f, 4.f) * arx::randomVec3f();
		pd->siz = 7.f;
		pd->tolive = Random::getu(500, 1500);
		pd->m_flags = FIRE_TO_SMOKE | ROTATING;
		pd->tc = g_particleTextures.fire2;
		pd->m_rotation = Random::getf(-0.1f, 0.1f);
		pd->scale = Vec3f(-8.f);
		pd->rgb = Color3f(0.71f, 0.43f, 0.29f);
		pd->delay = nn * delay;
	}
}

void createObjFireParticles(const EERIE_3DOBJ * obj, int particlePositions, int perPos, int delay) {
	
	for(int i = 0; i < particlePositions; i++) {
		
		long notok = 10;
		std::vector<EERIE_FACE>::const_iterator it;
		
		while(notok-- > 0) {
			it = Random::getIterator(obj->facelist);
			arx_assert(it != obj->facelist.end());
			
			if(it->facetype & POLY_HIDE)
				continue;
			
			notok = -1;
		}
		
		if(notok < 0) {
			Vec3f pos = obj->vertexWorldPositions[it->vid[0]].v;
			
			createFireParticles(pos, perPos, delay);
		}
	}
}


void ARX_PARTICLES_Spawn_Lava_Burn(Vec3f pos, Entity * io) {
	
	if(io && io->obj && !io->obj->facelist.empty()) {
		size_t num = 0;
		long notok = 10;
		while(notok-- > 0) {
			num = Random::getu(0, io->obj->facelist.size() - 1);
			if(io->obj->facelist[num].facetype & POLY_HIDE) {
				continue;
			}
			if(glm::abs(pos.y - io->obj->vertexWorldPositions[io->obj->facelist[num].vid[0]].v.y) > 50.f) {
				continue;
			}
			notok = -1;
		}
		pos = io->obj->vertexWorldPositions[io->obj->facelist[num].vid[0]].v;
	}
	
	PARTICLE_DEF * pd = createParticle();
	if(!pd) {
		return;
	}
	
	pd->ov = pos;
	pd->move = arx::randomVec3f() * Vec3f(2.f, -12.f, 2.f) - Vec3f(4.f, 15.f, 4.f);
	pd->tolive = 800;
	pd->tc = g_particleTextures.smoke;
	pd->siz = 15.f;
	pd->scale = arx::randomVec(15.f, 20.f);
	pd->m_flags = FIRE_TO_SMOKE;
	if(Random::getf() > 0.5f) {
		pd->m_flags |= SUBSTRACT;
	}
}

static void ARX_PARTICLES_Spawn_Rogue_Blood(const Vec3f & pos, float dmgs, Color col) {
	
	PARTICLE_DEF * pd = createParticle();
	if(!pd) {
		return;
	}
	
	pd->ov = pos;
	pd->siz = 3.1f * (dmgs * (1.f / 60) + .9f);
	pd->scale = Vec3f(-pd->siz * 0.25f);
	pd->m_flags = PARTICLE_SUB2 | SUBSTRACT | GRAVITY | ROTATING | SPLAT_GROUND;
	pd->tolive = 1600;
	pd->move = arx::randomVec3f() * Vec3f(60.f, -10.f, 60.f) - Vec3f(30.f, 15.f, 30.f);
	pd->rgb = Color3f(col);
	long num = Random::get(0, 5);
	pd->tc = g_particleTextures.bloodsplat[num];
	pd->m_rotation = Random::getf(-0.05f, 0.05f);
	
}

static void ARX_PARTICLES_Spawn_Blood3(const Vec3f & pos, float dmgs, Color col,
                                       ParticlesTypeFlags flags = 0) {
	
	PARTICLE_DEF * pd = createParticle();
	if(pd) {
		float sinW = timeWaveSin(g_gameTime.now(), 2s * glm::pi<float>());
		float cosW = timeWaveCos(g_gameTime.now(), 2s * glm::pi<float>());
		float power = dmgs * (1.f / 60) + .9f;
		pd->ov = pos + Vec3f(-sinW, sinW, cosW) * 30.f;
		pd->siz = 3.5f * power + sinW;
		pd->scale = Vec3f(-pd->siz * 0.5f);
		pd->m_flags = PARTICLE_SUB2 | SUBSTRACT | GRAVITY | ROTATING | flags;
		pd->tolive = 1100;
		pd->rgb = Color3f(col);
		pd->tc = g_particleTextures.bloodsplat[0];
		pd->m_rotation = Random::getf(-0.05f, 0.05f);
	}
	
	if(Random::getf() > .90f) {
		ARX_PARTICLES_Spawn_Rogue_Blood(pos, dmgs, col);
	}
	
}



void ARX_PARTICLES_Spawn_Blood2(const Vec3f & pos, float dmgs, Color col, Entity * io) {
	
	bool isNpc = io && (io->ioflags & IO_NPC);
	
	if(isNpc && io->_npcdata->SPLAT_TOT_NB) {
		
		if(io->_npcdata->SPLAT_DAMAGES < 3) {
			return;
		}
		
		float power = (io->_npcdata->SPLAT_DAMAGES * (1.f / 60)) + .9f;
		
		Vec3f vect = pos - io->_npcdata->last_splat_pos;
		float dist = glm::length(vect);
		vect = glm::normalize(vect);
		long nb = long(dist / 4.f * power);
		if(nb == 0) {
			nb = 1;
		}
		
		long MAX_GROUND_SPLATS;
		switch(config.video.levelOfDetail) {
			case 2:  MAX_GROUND_SPLATS = 10; break;
			case 1:  MAX_GROUND_SPLATS = 5; break;
			default: MAX_GROUND_SPLATS = 1; break;
		}
		
		for(long k = 0; k < nb; k++) {
			Vec3f posi = io->_npcdata->last_splat_pos + vect * Vec3f(k * 4.f * power);
			io->_npcdata->SPLAT_TOT_NB++;
			if(io->_npcdata->SPLAT_TOT_NB > MAX_GROUND_SPLATS) {
				ARX_PARTICLES_Spawn_Blood3(posi, io->_npcdata->SPLAT_DAMAGES, col, SPLAT_GROUND);
				io->_npcdata->SPLAT_TOT_NB = 1;
			} else {
				ARX_PARTICLES_Spawn_Blood3(posi, io->_npcdata->SPLAT_DAMAGES, col);
			}
		}
		
	} else {
		if(isNpc) {
			io->_npcdata->SPLAT_DAMAGES = short(dmgs);
		}
		ARX_PARTICLES_Spawn_Blood3(pos, dmgs, col, SPLAT_GROUND);
		if(isNpc) {
			io->_npcdata->SPLAT_TOT_NB = 1;
		}
	}
	
	if(isNpc) {
		io->_npcdata->last_splat_pos = pos;
	}
}

void ARX_PARTICLES_Spawn_Blood(const Vec3f & pos, float dmgs, EntityHandle source) {
	
	Entity * sourceIo = entities.get(source);
	if(!sourceIo) {
		return;
	}
	
	float nearest_dist = std::numeric_limits<float>::max();
	long nearest = -1;
	long count = sourceIo->obj->grouplist.size();
	for(long i = 0; i < count; i += 2) {
		long vertex = sourceIo->obj->grouplist[i].origin;
		float dist = arx::distance2(pos, sourceIo->obj->vertexWorldPositions[vertex].v);
		if(dist < nearest_dist) {
			nearest_dist = dist;
			nearest = i;
		}
	}
	if(nearest < 0) {
		return;
	}
	
	// Decides number of blood particles...
	const unsigned int spawn_nb = glm::clamp(long(dmgs * 2.f), 5l, 26l);
	
	unsigned long totdelay = 0;
	
	for(unsigned int k = 0; k < spawn_nb; k++) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			return;
		}
		
		pd->siz = 0.f;
		pd->scale = Vec3f(float(spawn_nb));
		pd->m_flags = GRAVITY | ROTATING | DELAY_FOLLOW_SOURCE;
		pd->source = &sourceIo->obj->vertexWorldPositions[nearest].v;
		pd->sourceionum = source;
		pd->tolive = 1200 + spawn_nb * 5;
		totdelay += 45 + Random::getu(0, 150 - spawn_nb);
		pd->delay = totdelay;
		pd->rgb = Color3f(.9f, 0.f, 0.f);
		pd->tc = g_particleTextures.bloodsplat[0];
		pd->m_rotation = Random::getf(-0.05f, 0.05f);
	}
}


void MakeCoolFx(const Vec3f & pos) {
	spawnFireHitParticle(pos, 1);
	PolyBoomAddScorch(pos);
}

void MakePlayerAppearsFX(const Entity & io) {
	MakeCoolFx(io.pos);
	MakeCoolFx(io.pos);
	AddRandomSmoke(io, 30);
	ARX_PARTICLES_Add_Smoke(io.pos, 1 | 2, 20); // flag 1 = randomize pos
}

void AddRandomSmoke(const Entity & io, long amount) {
	
	for(size_t i = 0; i < size_t(amount); i++) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			return;
		}
		
		size_t vertex = Random::get(size_t(0), io.obj->vertexlist.size() - 1);
		pd->ov = io.obj->vertexWorldPositions[vertex].v + arx::randomVec(-5.f, 5.f);
		pd->siz = Random::getf(0.f, 8.f);
		if(pd->siz < 4.f) {
			pd->siz = 4.f;
		}
		pd->scale = Vec3f(10.f);
		pd->m_flags = ROTATING | FADE_IN_AND_OUT;
		pd->tolive = Random::getu(900, 1300);
		pd->move = arx::linearRand(Vec3f(-0.25f, -0.7f, -0.25f), Vec3f(0.25f, 0.3f, 0.25f));
		pd->rgb = Color3f(0.3f, 0.3f, 0.34f);
		pd->tc = g_particleTextures.smoke;
		pd->m_rotation = 0.001f;
	}
}

// flag 1 = randomize pos
void ARX_PARTICLES_Add_Smoke(const Vec3f & pos, long flags, long amount, const Color3f & rgb) {
	
	Vec3f mod = (flags & 1) ? arx::randomVec(-50.f, 50.f) : Vec3f(0.f);
	
	while(amount--) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			return;
		}
		
		pd->ov = pos + mod;
		if(flags & 2) {
			pd->siz = Random::getf(15.f, 35.f);
			pd->scale = arx::randomVec(40.f, 55.f);
		} else {
			pd->siz = Random::getf(5.f, 13.f);
			pd->scale = arx::randomVec(10.f, 15.f);
		}
		pd->m_flags = ROTATING | FADE_IN_AND_OUT;
		pd->tolive = Random::getu(1100, 1500);
		pd->delay = amount * 120 + Random::getu(0, 100);
		pd->move = arx::linearRand(Vec3f(-0.25f, -0.7f, -0.25f), Vec3f(0.25f, 0.3f, 0.25f));
		pd->rgb = rgb;
		pd->tc = g_particleTextures.smoke;
		pd->m_rotation = 0.01f;
	}
}

void ManageTorch() {
	arx_assert(entities.player());
	
	EERIE_LIGHT * el = lightHandleGet(torchLightHandle);
	
	if(player.torch) {
		
		float rr = Random::getf();
		el->pos = player.pos;
		el->intensity = 1.6f;
		el->fallstart = 280.f + rr * 20.f;
		el->fallend = el->fallstart + 280.f;
		el->m_exists = true;
		el->rgb = player.m_torchColor - Color3f(rr, rr, rr) * Color3f(0.1f, 0.1f, 0.1f);
		el->duration = 0;
		el->extras = 0;
		
	} else if(cur_mr == 3) {
		
		el->pos = player.pos;
		el->intensity = 1.8f;
		el->fallstart = 480.f;
		el->fallend = el->fallstart + 480.f;
		el->m_exists = true;
		el->rgb = Color3f(1.f, .5f, .8f);
		el->duration = 0;
		el->extras = 0;
		
	} else {
		
		long count = MagicFlareCountNonFlagged();
		
		if(count) {
			float rr = Random::getf();
			el->pos = player.pos;
			el->fallstart = 140.f + float(count) * 0.333333f + rr * 5.f;
			el->fallend = 220.f + float(count) * 0.5f + rr * 5.f;
			el->intensity = 1.6f;
			el->m_exists = true;
			el->rgb = Color3f(0.01f * count, 0.009f * count, 0.008f * count);
		} else {
			el->m_exists = false;
		}
	}
	
	if(   entities.player()->obj
	   && entities.player()->obj->fastaccess.head_group_origin != ObjVertHandle()
	) {
		s32 vertex = entities.player()->obj->fastaccess.head_group_origin.handleData();
		el->pos.y = entities.player()->obj->vertexWorldPositions[vertex].v.y;
	}
}

void Add3DBoom(const Vec3f & position) {
	
	Vec3f poss = position;
	ARX_SOUND_PlaySFX(g_snd.SPELL_FIRE_HIT, &poss);
	
	{
		float dist = fdist(player.pos - Vec3f(0, 160.f, 0.f), position);
		if(dist < 300) {
			Vec3f vect = (player.pos - position - Vec3f(0.f, 160.f, 0.f)) / dist;
			player.physics.forces += vect * ((300.f - dist) * 0.0125f);
		}
	}
	
	for(Entity & item : entities.inScene(IO_ITEM)) {
		
		if(!item.obj || !item.obj->pbox) {
			continue;
		}
		
		for(size_t k = 0; k < item.obj->pbox->vert.size(); k++) {
			float dist = fdist(item.obj->pbox->vert[k].pos, position);
			if(dist < 300.f) {
				item.obj->pbox->active = 1;
				item.obj->pbox->stopcount = 0;
				Vec3f vect = (item.obj->pbox->vert[k].pos - position) / dist;
				item.obj->pbox->vert[k].velocity += vect * ((300.f - dist) * 10.f);
			}
		}
		
	}
}

void ARX_PARTICLES_FirstInit() {
	
}

void ARX_PARTICLES_ClearAll() {
	
	std::fill(g_particles, g_particles + MAX_PARTICLES, PARTICLE_DEF());
	ParticleCount = 0;
}

PARTICLE_DEF * createParticle(bool allocateWhilePaused) {
	
	if(!allocateWhilePaused && g_gameTime.isPaused()) {
		return nullptr;
	}
	
	for(size_t i = 0; i < MAX_PARTICLES; i++) {
		
		PARTICLE_DEF * pd = &g_particles[i];
		
		if(pd->exist) {
			continue;
		}
		
		ParticleCount++;
		pd->exist = true;
		pd->timcreation = toMsi(g_gameTime.now());
		
		pd->is2D = false;
		pd->rgb = Color3f::white;
		pd->tc = nullptr;
		pd->m_flags = 0;
		pd->source = nullptr;
		pd->delay = 0;
		pd->zdec = false;
		pd->move = Vec3f(0.f);
		pd->scale = Vec3f(1.f);
		
		return pd;
	}
	
	return nullptr;
}

void MagFX(const Vec3f & pos, float size) {
	
	PARTICLE_DEF * pd = createParticle();
	if(!pd) {
		return;
	}
	
	pd->ov = pos + Vec3f(Random::getf(0.f, 6.f) - Random::getf(0.f, 12.f), Random::getf(0.f, 6.f) - Random::getf(0.f, 12.f), 0.f);
	pd->move = Vec3f(Random::getf(-6.f, 6.f), Random::getf(-8.f, 8.f), 0.f);
	pd->scale = Vec3f(4.4f, 4.4f, 1.f);
	pd->tolive = Random::getu(1500, 2400);
	pd->tc = g_particleTextures.healing;
	pd->rgb = Color3f::magenta;
	pd->siz = 56.f * size;
	pd->is2D = true;
}

void ARX_PARTICLES_Spawn_Splat(const Vec3f & pos, float dmgs, Color col) {
	
	const unsigned long tolive = 1000 + static_cast<unsigned long>(dmgs) * 3;
	
	float power = (dmgs * (1.f / 60)) + .9f;
	
	for(long kk = 0; kk < 20; kk++) {
		
		PARTICLE_DEF * pd = createParticle(true);
		if(!pd) {
			return;
		}
		
		pd->m_flags = PARTICLE_SUB2 | SUBSTRACT | GRAVITY;
		pd->ov = pos;
		pd->move = arx::randomVec(-11.5f, 11.5f);
		pd->tolive = tolive;
		pd->tc = g_particleTextures.blood_splat;
		pd->siz = 0.3f + 0.01f * power;
		pd->scale = Vec3f(0.2f + 0.3f * power);
		pd->zdec = true;
		pd->rgb = Color3f(col);
	}
}

void ARX_PARTICLES_SpawnWaterSplash(const Vec3f & _ePos) {
	
	long nbParticles = Random::get(15, 35);
	for(long kk = 0; kk < nbParticles; kk++) {
		
		PARTICLE_DEF * pd = createParticle(true);
		if(!pd) {
			return;
		}
		
		pd->m_flags = FADE_IN_AND_OUT | ROTATING | DISSIPATING | GRAVITY | SPLAT_WATER;
		pd->m_rotation = 0.f; // TODO maybe remove ROTATING
		pd->ov = _ePos + Vec3f(30.f, -20.f, 30.f) * arx::randomVec3f();
		pd->move = arx::linearRand(Vec3f(-6.5f, -11.5f, -6.5f), Vec3f(6.5f, 0.f, 6.5f));
		pd->tolive = Random::getu(1000, 1300);
		pd->tc = g_particleTextures.water_drop[Random::get(0, 2)];
		pd->siz = 0.4f;
		float s = Random::getf();
		pd->zdec = true;
		pd->rgb = Color3f::gray(s);
		
	}
	
}

void SpawnFireballTail(const Vec3f & poss, const Vec3f & vecto, float level, long flags) {
	
	if(!g_particleTextures.explo[0]) {
		return;
	}
	
	for(long nn = 0; nn < 2; nn++) {
		
		PARTICLE_DEF * pd = createParticle(true);
		if(!pd) {
			return;
		}
		
		pd->m_flags = FIRE_TO_SMOKE | FADE_IN_AND_OUT | PARTICLE_ANIMATED | ROTATING;
		pd->m_rotation = Random::getf(0.f, 0.02f);
		pd->move = Vec3f(0.f, Random::getf(-3.f, 0.f), 0.f);
		pd->tc = g_particleTextures.explo[0];
		pd->rgb = Color3f::gray(.7f);
		pd->siz = (level + Random::getf()) * 2.f;
		
		if(flags & 1) {
			pd->tolive = Random::getu(400, 500);
			pd->siz *= 0.7f;
			pd->scale = Vec3f(level * 1.4f);
		} else {
			pd->scale = Vec3f(level * 2.f);
			pd->tolive = Random::getu(800, 900);
		}
		
		pd->cval1 = 0;
		pd->cval2 = g_particleTextures.MAX_EXPLO - 1;
		
		if(nn == 1) {
			pd->delay = Random::getu(150, 250);
			pd->ov = poss + vecto * Vec3f(pd->delay);
		} else {
			pd->ov = poss;
		}
	}
}

void LaunchFireballBoom(const Vec3f & poss, float level, Vec3f * direction, Color3f * rgb) {
	
	level *= 1.6f;
	
	if(g_particleTextures.explo[0] == nullptr) {
		return;
	}
	
	PARTICLE_DEF * pd = createParticle(true);
	if(!pd) {
		return;
	}
	
	pd->m_flags = FIRE_TO_SMOKE | FADE_IN_AND_OUT | PARTICLE_ANIMATED;
	pd->ov = poss;
	pd->move = (direction) ? *direction : Vec3f(0.f, Random::getf(-5.f, 0.f), 0.f);
	pd->tolive = Random::getu(1600, 2200);
	pd->tc = g_particleTextures.explo[0];
	pd->siz = level * 3.f + Random::getf(0.f, 2.f);
	pd->scale = Vec3f(level * 3.f);
	pd->zdec = true;
	pd->cval1 = 0;
	pd->cval2 = g_particleTextures.MAX_EXPLO - 1;
	if(rgb) {
		pd->rgb = *rgb;
	}
	
}

void spawnFireHitParticle(const Vec3f & poss, long type) {
	
	PARTICLE_DEF * pd = createParticle(true);
	if(pd) {
		pd->ov = poss;
		pd->move = Vec3f(3.f, 4.f, 3.f) - Vec3f(6.f, 12.f, 6.f) * arx::randomVec3f();
		pd->tolive = Random::getu(600, 700);
		pd->tc = g_particleTextures.fire_hit;
		pd->siz = Random::getf(100.f, 110.f) * ((type == 1) ? 2.f : 1.f);
		pd->zdec = true;
		if(type == 1) {
			pd->rgb = Color3f(.4f, .4f, 1.f);
		}
		
		pd = createParticle(true);
		if(pd) {
			pd->ov = poss;
			pd->move = Vec3f(3.f , 4.f, 3.f) - Vec3f(6.f, 12.f, 6.f) * arx::randomVec3f();
			pd->tolive = Random::getu(600, 700);
			pd->tc = g_particleTextures.fire_hit;
			pd->siz = Random::getf(40.f, 70.f) * ((type == 1) ? 2.f : 1.f);
			pd->zdec = true;
			if(type == 1) {
				pd->rgb = Color3f(.4f, .4f, 1.f);
			}
		}
		
	}
}

void spawn2DFireParticle(const Vec2f & pos, float scale) {
	
	PARTICLE_DEF * pd = createParticle();
	if(!pd) {
		return;
	}
	
	pd->m_flags = FIRE_TO_SMOKE;
	pd->ov = Vec3f(pos, 0.0000001f);
	pd->move = Vec3f(Random::getf(-1.5f, 1.5f), Random::getf(-6.f, -5.f), 0.f) * scale;
	pd->scale = Vec3f(1.8f, 1.8f, 1.f);
	pd->tolive = Random::getu(500, 900);
	pd->tc = g_particleTextures.fire2;
	pd->rgb = Color3f(1.f, .6f, .5f);
	pd->siz = 14.f * scale;
	pd->is2D = true;
}



void ARX_PARTICLES_Update()  {
	
	ARX_PROFILE_FUNC();
	
	if(!g_tiles) {
		return;
	}
	
	if(ParticleCount == 0) {
		return;
	}
	
	const GameInstant now = g_gameTime.now();
	
	long pcc = ParticleCount;
	
	for(size_t i = 0; i < MAX_PARTICLES && pcc > 0; i++) {

		PARTICLE_DEF * part = &g_particles[i];
		if(!part->exist) {
			continue;
		}

		long framediff = part->timcreation + part->tolive - toMsi(now);
		long framediff2 = toMsi(now) - part->timcreation;
		
		if(framediff2 < long(part->delay)) {
			continue;
		}
		
		if(part->delay > 0) {
			part->timcreation += part->delay;
			part->delay = 0;
			Entity * target = entities.get(part->sourceionum);
			if((part->m_flags & DELAY_FOLLOW_SOURCE) && target) {
				part->ov = *part->source;
				Vec3f vector = (part->ov - target->pos) * Vec3f(1.f, 0.5f, 1.f);
				vector = glm::normalize(vector);
				part->move = vector * Vec3f(18.f, 5.f, 18.f) + arx::randomVec(-0.5f, 0.5f);
			}
			continue;
		}
		
		if(!part->is2D && !g_tiles->isInActiveTile(part->ov)) {
			part->exist = false;
			ParticleCount--;
			continue;
		}
		
		if(framediff <= 0) {
			if((part->m_flags & FIRE_TO_SMOKE) && Random::getf() > 0.7f) {
				
				part->ov += part->move;
				part->tolive = u32(part->tolive * 1.375f);
				part->m_flags &= ~FIRE_TO_SMOKE;
				part->tc = g_particleTextures.smoke;
				part->scale = glm::abs(part->scale * 2.4f);
				part->rgb = Color3f::gray(.45f);
				part->move *= 0.5f;
				part->siz *= 1.f / 3;
				part->timcreation = toMsi(now);
				
				framediff = part->tolive;
				
			} else {
				part->exist = false;
				ParticleCount--;
				continue;
			}
		}
		
		float val = (part->tolive - framediff) * 0.01f;
		
		Vec3f in = part->ov + part->move * val;
		Vec3f inn = in;
		
		if(part->m_flags & GRAVITY) {
			in.y = inn.y = inn.y + 1.47f * val * val;
		}
		
		float fd = float(framediff2) / float(part->tolive);
		float r = 1.f - fd;
		if(part->m_flags & FADE_IN_AND_OUT) {
			long t = part->tolive / 2;
			if(framediff2 <= t) {
				r = float(framediff2) / float(t);
			} else {
				r = 1.f - float(framediff2 - t) / float(t);
			}
		}
		
		if(!part->is2D) {
			
			Sphere sp;
			sp.origin = in;
			
			Vec4f p = worldToClipSpace(inn);
			float z = p.z / p.w;
			if(p.w <= 0.f || z > g_camera->cdepth * fZFogEnd) {
				continue;
			}
			
			if(part->m_flags & SPLAT_GROUND) {
				float siz = part->siz + part->scale.x * fd;
				sp.radius = siz * 10.f;
				if(CheckAnythingInSphere(sp, entities.player(), CAS_NO_NPC_COL)) {
					if(Random::getf() < 0.9f) {
						Color3f rgb = part->rgb;
						PolyBoomAddSplat(sp, rgb, 0);
					}
					part->exist = false;
					ParticleCount--;
					continue;
				}
			}
			
			if(part->m_flags & SPLAT_WATER) {
				float siz = part->siz + part->scale.x * fd;
				sp.radius = siz * Random::getf(10.f, 30.f);
				if(CheckAnythingInSphere(sp, entities.player(), CAS_NO_NPC_COL)) {
					if(Random::getf() < 0.9f) {
						Color3f rgb = part->rgb * 0.5f;
						PolyBoomAddSplat(sp, rgb, 2);
					}
					part->exist = false;
					ParticleCount--;
					continue;
				}
			}
			
			if((part->m_flags & DISSIPATING) && z < 0.05f) {
				r *= z * 20.f;
			}
		}
		
		if(r <= 0.f) {
			pcc--;
			continue;
		}
		
		if(part->m_flags & PARTICLE_GOLDRAIN) {
			float v = Random::getf(-0.1f, 0.1f);
			if(part->rgb.r + v <= 1.f && part->rgb.r + v > 0.f
				&& part->rgb.g + v <= 1.f && part->rgb.g + v > 0.f
				&& part->rgb.b + v <= 1.f && part->rgb.b + v > 0.f) {
				part->rgb = Color3f(part->rgb.r + v, part->rgb.g + v, part->rgb.b + v);
			}
		}
		
		Color color(part->rgb * r);
		if(player.m_improve) {
			color.g = 0;
		}
		
		TextureContainer * tc = part->tc;
		if(tc == g_particleTextures.explo[0] && (part->m_flags & PARTICLE_ANIMATED)) {
			long animrange = part->cval2 - part->cval1;
			long num = long(float(framediff2) / float(part->tolive) * animrange);
			num = glm::clamp(num, long(part->cval1), long(part->cval2));
			tc = g_particleTextures.explo[num];
		}
		
		float siz = part->siz + part->scale.x * fd;

		RenderMaterial mat;
		mat.setTexture(tc);
		mat.setDepthTest(!(part->m_flags & PARTICLE_NOZBUFFER));
		
		if(part->m_flags & PARTICLE_SUB2) {
			mat.setBlendType(RenderMaterial::Subtractive2);
			color.a = Color::Traits::convert(r * 1.5f);
		} else if(part->m_flags & SUBSTRACT) {
			mat.setBlendType(RenderMaterial::Subtractive);
		} else {
			mat.setBlendType(RenderMaterial::Additive);
		}
		
		if(part->m_flags & ROTATING) {
			if(!part->is2D) {
				float rott = MAKEANGLE(float(toMsi(now) + framediff2) * part->m_rotation);
				
				float temp = (part->zdec) ? 0.0001f : 2.f;
				float size = std::max(siz, 0.f);
				EERIEAddSprite(mat, in, size, color, temp, rott);
				
			}
		} else if(part->is2D) {
			
			float siz2 = part->siz + part->scale.y * fd;
			EERIEAddBitmap(mat, in, siz, siz2, tc, color);
			
		} else {
			
			float temp = (part->zdec) ? 0.0001f : 2.f;
			EERIEAddSprite(mat, in, siz, color, temp);
			
		}
		
		pcc--;
	}
}

void RestoreAllLightsInitialStatus() {
	
	for(EERIE_LIGHT & light : g_staticLights) {
		light.m_ignitionStatus = !(light.extras & EXTRAS_STARTEXTINGUISHED);
		if(!light.m_ignitionStatus) {
			lightHandleDestroy(light.m_ignitionLightHandle);
		}
	}
	
}

// Draws Flame Particles
void TreatBackgroundActions() {
	
	ARX_PROFILE_FUNC();
	
	float fZFar = square(g_camera->cdepth * fZFogEnd * 1.3f);
	
	for(EERIE_LIGHT & light : g_staticLights) {
		
		float dist = arx::distance2(light.pos, g_camera->m_pos);
		if(dist > fZFar) {
			// Out of treat range
			ARX_SOUND_Stop(light.sample);
			light.sample = audio::SourcedSample();
			DamageRequestEnd(light.m_damage);
			light.m_damage = DamageHandle();
			continue;
		}
		
		if((light.extras & EXTRAS_SPAWNFIRE) && light.m_ignitionStatus) {
			Spell * spell = nullptr; // TODO create a real spell for this?
			DamageParameters & damage = damageGet(spell, light.m_damage);
			damage.radius = light.ex_radius;
			damage.damages = light.ex_radius * 0.4f;
			damage.area = DAMAGE_FULL;
			damage.duration = GameDuration::max();
			damage.source = EntityHandle();
			damage.flags = 0;
			damage.type = DAMAGE_TYPE_FAKESPELL | DAMAGE_TYPE_MAGICAL | DAMAGE_TYPE_FIRE | DAMAGE_TYPE_NO_FIX;
			damage.pos = light.pos;
		} else if(light.m_damage != DamageHandle()) {
			DamageRequestEnd(light.m_damage);
			light.m_damage = DamageHandle();
		}
		
		if(!(light.extras & (EXTRAS_SPAWNFIRE | EXTRAS_SPAWNSMOKE)) || !light.m_ignitionStatus) {
			if(!light.m_ignitionStatus) {
				ARX_SOUND_Stop(light.sample);
				light.sample = audio::SourcedSample();
			}
			continue;
		}
		
		if(light.sample == audio::SourcedSample()) {
			light.sample = ARX_SOUND_PlaySFX_loop(g_snd.FIREPLACE_LOOP, &light.pos, Random::getf(0.95f, 1.05f));
		} else {
			ARX_SOUND_RefreshPosition(light.sample, light.pos);
		}
		
		float amount = 2.f;
		if(dist < square(g_camera->cdepth * (1.f / 6))) {
			amount = 3.f;
		} else if(dist < square(g_camera->cdepth * (1.f / 3))) {
			amount = 2.5f;
		}
		const float targetFPS = 61.f;
		const float targetDelay = 1000.f / targetFPS;
		long count = light.m_storedFlameTime.update(amount * g_framedelay * (1.f / targetDelay));
		
		for(long n = 0; n < count; n++) {
			
			if(Random::getf() < light.ex_frequency) {
				PARTICLE_DEF * pd = createParticle();
				if(pd) {
					float t = Random::getf() * glm::pi<float>();
					Vec3f s = Vec3f(std::sin(t), std::sin(t), std::cos(t)) * arx::randomVec();
					pd->ov = light.pos + s * light.ex_radius;
					pd->move = Vec3f(2.f, 2.f, 2.f) - Vec3f(4.f, 22.f, 4.f) * arx::randomVec3f();
					pd->move *= light.ex_speed;
					pd->siz = 7.f * light.ex_size;
					pd->tolive = 500 + Random::getu(0, unsigned(1000 * light.ex_speed));
					if((light.extras & EXTRAS_SPAWNFIRE) && (light.extras & EXTRAS_SPAWNSMOKE)) {
						pd->m_flags = FIRE_TO_SMOKE;
					}
					pd->tc = (light.extras & EXTRAS_SPAWNFIRE) ? g_particleTextures.fire2 : g_particleTextures.smoke;
					pd->m_flags |= ROTATING;
					pd->m_rotation = 0.1f - Random::getf(0.f, 0.2f) * light.ex_speed;
					pd->scale = Vec3f(-8.f);
					pd->rgb = (light.extras & EXTRAS_COLORLEGACY) ? light.rgb : Color3f::white;
				}
			}
			
			if(!(light.extras & EXTRAS_SPAWNFIRE) || Random::getf() <= 0.95f) {
				continue;
			}
			
			if(Random::getf() < light.ex_frequency) {
				PARTICLE_DEF * pd = createParticle();
				if(pd) {
					float t = Random::getf() * (glm::pi<float>() * 2.f) - glm::pi<float>();
					Vec3f s = Vec3f(std::sin(t), std::sin(t), std::cos(t)) * arx::randomVec();
					pd->ov = light.pos + s * light.ex_radius;
					Vec3f vect = glm::normalize(pd->ov - light.pos);
					float d = (light.extras & EXTRAS_FIREPLACE) ? 6.f : 4.f;
					pd->move = Vec3f(vect.x * d, Random::getf(-18.f, -10.f), vect.z * d) * light.ex_speed;
					pd->siz = 4.f * light.ex_size * 0.3f;
					pd->tolive = 1200 + Random::getu(0, unsigned(500 * light.ex_speed));
					pd->tc = g_particleTextures.fire2;
					pd->m_flags |= ROTATING | GRAVITY;
					pd->m_rotation = 0.1f - Random::getf(0.f, 0.2f) * light.ex_speed;
					pd->scale = Vec3f(-3.f);
					pd->rgb = (light.extras & EXTRAS_COLORLEGACY) ? light.rgb : Color3f::white;
				}
			}
			
		}
		
	}
	
}

