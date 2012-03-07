#ifndef BASICAI_H
#define BASICAI_H

#include <BWAPI.h>

#include <BWTA.h>
#include <windows.h>

extern bool analyzed;
DWORD WINAPI AnalyzeThread();

class BasicAI : public BWAPI::AIModule
{
	BWTA::BaseLocation* natural;

	BWAPI::Unit* main_base;
	BWAPI::Unit* natural_base;

	int overlords_building;
	int overlords_finished;
	int overlords_destroyed;

	int reserved_minerals;

	int past_supply;
public:
	virtual void onStart();
	virtual void onEnd(bool isWinner);
	virtual void onFrame();
	virtual void onSendText(std::string text);
	virtual void onReceiveText(BWAPI::Player* player, std::string text);
	virtual void onPlayerLeft(BWAPI::Player* player);
	virtual void onNukeDetect(BWAPI::Position target);
	virtual void onUnitDiscover(BWAPI::Unit* unit);
	virtual void onUnitEvade(BWAPI::Unit* unit);
	virtual void onUnitShow(BWAPI::Unit* unit);
	virtual void onUnitHide(BWAPI::Unit* unit);
	virtual void onUnitCreate(BWAPI::Unit* unit);
	virtual void onUnitDestroy(BWAPI::Unit* unit);
	virtual void onUnitMorph(BWAPI::Unit* unit);
	virtual void onUnitRenegade(BWAPI::Unit* unit);
	virtual void onSaveGame(std::string gameName);
	virtual void onUnitComplete(BWAPI::Unit *unit);

	void morphLarva(BWAPI::Unit* base, BWAPI::UnitType unit);

	int availableMins();
	int availableSupply();
	void updateSupply();

	void onAnalyze();

	void drawVisibilityData();
	void drawTerrainData();
};

#endif