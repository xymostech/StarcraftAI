#include "StarcraftAI.h"
using namespace BWAPI;

bool analyzed;

void StarcraftAI::morphLarva(Unit* base, BWAPI::UnitType unit) {
	int mins = Broodwar->self()->minerals() - reserved_minerals;
	int supply = Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed();
	Unit* larva = *base->getLarva().begin();
		
	if (unit == UnitTypes::Zerg_Overlord) {
		if (mins >= 100) {
			larva->morph(unit);
		}
	} else if (unit == UnitTypes::Zerg_Drone) {
		if (supply >= 2 && mins >= 50) {
			larva->morph(unit);
		}
	} else if (unit == UnitTypes::Zerg_Zergling) {
		if (supply >= 2 && mins >= 50) {
			larva->morph(unit);
		}
	}
}

int StarcraftAI::availableMins() {
	return Broodwar->self()->minerals() - reserved_minerals;
}

int StarcraftAI::availableSupply() {
	return Broodwar->self()->supplyTotal()
		   - Broodwar->self()->supplyUsed()
		   + 16 * supply_building
		   + 16 * supply_finished
		   - 16 * supply_destroyed;
}

void StarcraftAI::updateSupply() {
	while (supply_finished > 0 &&
		    supply_destroyed > 0) {
		supply_finished--;
		supply_destroyed--;
	}

	int supply = Broodwar->self()->supplyTotal();

	if (supply < past_supply) {
		supply_destroyed -= (past_supply - supply) / 16;
	} else if (supply > past_supply) {
		supply_finished -= (supply - past_supply) / 16;
	}

	past_supply = supply;
}

void StarcraftAI::onAnalyze() {
	natural = NULL;
	double best_dist;
	BWAPI::Position my_pos = BWTA::getStartLocation(Broodwar->self())->getPosition();

	std::set<BWTA::BaseLocation*>::const_iterator b_it, b_end = BWTA::getBaseLocations().end();
	for (b_it = BWTA::getBaseLocations().begin(); b_it != b_end; ++b_it) {
		if ((!natural ||
			(*b_it)->getPosition().getDistance(my_pos) < best_dist) &&
			(*b_it) != BWTA::getStartLocation(Broodwar->self())) {
			best_dist = (*b_it)->getPosition().getDistance(my_pos);
			natural = (*b_it);
		}
	}
}

void StarcraftAI::drawVisibilityData()
{
}

void StarcraftAI::drawTerrainData()
{
	if (pool_pos) 
		Broodwar->drawBox(CoordinateType::Map,pool_pos.x()*32,pool_pos.y()*32,pool_pos.x()*32+3*32,pool_pos.y()*32+2*32,Colors::Orange,false);
	for (std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
	{
		for (std::set<BWTA::Chokepoint*>::const_iterator c=(*r)->getChokepoints().begin();c!=(*r)->getChokepoints().end();c++)
		{
			Position point1=(*c)->getSides().first;
			Position point2=(*c)->getSides().second;
			Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Red);
		}
	}
}

void StarcraftAI::onStart()
{
	Broodwar->enableFlag(Flag::UserInput);
	
	BWTA::readMap();
	analyzed = false;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnalyzeThread, NULL, 0, NULL);
	
	supply_building = 0;
	supply_finished = 0;
	supply_destroyed = 0;

	natural_base = NULL;
	spawning_pool = NULL;
	
	pool_pos = TilePositions::Invalid;
	
	natural = NULL;

	reserved_minerals = 0;

	past_supply = Broodwar->self()->supplyTotal();
}

void StarcraftAI::onEnd(bool isWinner)
{
}

bool noUnitsOn(const TilePosition& pos) {
	
	return (Broodwar->getUnitsOnTile(pos.x(), pos.y()).size() == 0);
}

void StarcraftAI::onFrame() {
 	updateSupply();
	
	if (analyzed) {
		onAnalyze();
		analyzed = false;
	}

	Unit* u;
	std::set<Unit*>::const_iterator it, end = Broodwar->self()->getUnits().end();

	for (it = Broodwar->self()->getUnits().begin(); it != end; ++it) {
		u = *it;
		 if (u->getType().isWorker()) {
			if (Broodwar->self()->minerals() >= 300 && !natural_base) {
				u->build(natural->getTilePosition(), UnitTypes::Zerg_Hatchery);
				reserved_minerals += 300;
				natural_base = u;
			} else if (Broodwar->self()->minerals() >= 200 && !spawning_pool) {
				TilePosition test_pos, move(1, 0);
				
				for (test_pos = main_base->getTilePosition();
				     !Broodwar->canBuildHere(u, test_pos, UnitTypes::Zerg_Spawning_Pool);
					 test_pos += move) {}
				
				u->build(test_pos, UnitTypes::Zerg_Spawning_Pool);
				reserved_minerals += 200;
				spawning_pool = u;
				
				pool_pos = test_pos;
				
			} else if (u->getOrder() == Orders::PlayerGuard) {
				Unit* closestMineral = NULL;
				int bestDistance = 0;
				std::set<Unit*>::iterator m_it, m_end = Broodwar->getMinerals().end();
				for (m_it = Broodwar->getMinerals().begin(); m_it != m_end; ++m_it) {
					if (closestMineral == NULL || 
						u->getDistance(*m_it) < bestDistance)\
					{
						closestMineral = *m_it;
						bestDistance = u->getDistance(*m_it);
					}
				}
				if (closestMineral != NULL) {
					u->rightClick(closestMineral);
				}
			}
		} else if (u->getType().isResourceDepot()) {
			if (!main_base) {
				main_base = u;
			}
			if (u->getLarva().size() > 0) {
				if (availableSupply() <= 2) {
					morphLarva(u, UnitTypes::Zerg_Overlord);
				} else {
					if (
						spawning_pool &&
					    !spawning_pool->isBeingConstructed() &&
						rand() > RAND_MAX / 2
						) {
						morphLarva(u, UnitTypes::Zerg_Zergling);
					} else {
						morphLarva(u, UnitTypes::Zerg_Drone);
					}
				}
			}
		} else if (u->getType() == UnitTypes::Zerg_Overlord) {
			if (natural) {
				u->move(natural->getPosition());
			}
		} else if (u->getType() == UnitTypes::Zerg_Zergling) {
			//u->attack(BWTA::getStartLocation(Broodwar->enemy())->getPosition());
		}
	}

	drawVisibilityData();
	drawTerrainData();
}

void StarcraftAI::onUnitMorph(BWAPI::Unit* unit) {
	if (unit->getType() == UnitTypes::Zerg_Overlord) {
		supply_building -= 1;
		supply_finished += 1;
	} else if (unit->getType() == UnitTypes::Zerg_Egg) {
		if (unit->getBuildType() == UnitTypes::Zerg_Overlord) {
			supply_building += 1;
		}
	} else if (unit->getType() == UnitTypes::Zerg_Hatchery) {
		reserved_minerals -= 300;
	} else if (unit->getType() == UnitTypes::Zerg_Spawning_Pool) {
		reserved_minerals -= 200;
	}
}

void StarcraftAI::onUnitDestroy(BWAPI::Unit* unit)
{
	if (unit->getType() == UnitTypes::Zerg_Overlord) {
		supply_destroyed += 1;
	}
}

void StarcraftAI::onSendText(std::string text)
{
	if (text == "/restart") {
		Broodwar->restartGame();
	}
}

void StarcraftAI::onReceiveText(BWAPI::Player* player, std::string text)
{
}

void StarcraftAI::onPlayerLeft(BWAPI::Player* player)
{
}

void StarcraftAI::onNukeDetect(BWAPI::Position target)
{
}

void StarcraftAI::onUnitDiscover(BWAPI::Unit* unit)
{
}

void StarcraftAI::onUnitEvade(BWAPI::Unit* unit)
{
}

void StarcraftAI::onUnitShow(BWAPI::Unit* unit)
{
}

void StarcraftAI::onUnitHide(BWAPI::Unit* unit)
{
}

void StarcraftAI::onUnitCreate(BWAPI::Unit* unit)
{
}

void StarcraftAI::onUnitRenegade(BWAPI::Unit* unit)
{
}

void StarcraftAI::onSaveGame(std::string gameName)
{
}

void StarcraftAI::onUnitComplete(BWAPI::Unit *unit)
{
}

DWORD WINAPI AnalyzeThread()
{
	BWTA::analyze();

	analyzed = true;
	return 0;
}