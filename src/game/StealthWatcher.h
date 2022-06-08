#ifndef ARX_GAME_STEALTH_H
#define ARX_GAME_STEALTH_H

#include "game/NPC.h"
#include "io/log/Logger.h"

class StealthWatcher {
private:
	//static StealthWatcher* swInstance;
	std::map<std::string, float> factors;
	float factorCache = -1.f;

public:
	static StealthWatcher& getInstance()
	{
		static StealthWatcher    instance;
		return instance;
	}

	/*static StealthWatcher* getInstance() {
		if (swInstance == NULL) {
			swInstance = new StealthWatcher();
		}
		return swInstance;
	}*/

	~StealthWatcher();

	void addUndetected(Entity& io, float distance, bool playerIsInFOV);

	void addDetected(Entity& io);

	void leaved(Entity& io);

	float getStealthBonusFactor();

	void evaluateStealthBonusFactor();

	void clear();
};

#endif // ARX_GAME_STEALTH_H
