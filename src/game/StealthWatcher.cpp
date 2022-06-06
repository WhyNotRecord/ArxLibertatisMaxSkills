#include "StealthWatcher.h"
#include "game/Player.h"

StealthWatcher::~StealthWatcher()
{
	clear();
}

void StealthWatcher::addUndetected(Entity& io, float distance, bool playerIsInFOV) {
	LogInfo << "Undetected by NPC: " << io.className() << ":" << io.id().string() << " dist: " << distance << " HP: " << io._npcdata->lifePool.current;
	float distanceFactor = MIN_STEALTH_DISTANCE_SQUARE / distance;
	float fovFactor = playerIsInFOV ? 2 : 1;
	if (distanceFactor > 5.f)
		distanceFactor = 5.f;
	float factor = io._npcdata->lifePool.current * distanceFactor * fovFactor * 0.001f;

	factors.insert_or_assign(io.id().string(), factor);
}

void StealthWatcher::addDetected(Entity& io) {
	if (factors.erase(io.id().string())) {
		LogInfo << "Detected by NPC: " << io.className() << ":" << io.id().string();
	}
}

void StealthWatcher::leaved(Entity& io) {
	if (factors.erase(io.id().string()))
		LogInfo << "NPC leaved: " << io.className() << ":" << io.id().string();
}


float StealthWatcher::getNearestEnemyFactor() {
	float result = 0.f;
	for (auto it = factors.begin(); it != factors.end(); ++it) {
		if (it->second > result)
			result = it->second;
	}
	return result > 1.f ? 1.f : result;
}

void StealthWatcher::clear() {
	factors.clear();
}
