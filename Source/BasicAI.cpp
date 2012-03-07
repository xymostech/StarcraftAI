
#include "BasicAI.h"
using namespace BWAPI;

bool analyzed;

void BasicAI::onStart()
{
	BWTA::readMap();
	analyzed = false;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AnalyzeThread, NULL, 0, NULL);
	
	overlords_building = 0;
	overlords_finished = 0;
	overlords_destroyed = 0;

	natural_base = NULL;
	
	natural = NULL;

	reserved_minerals = 0;

	past_supply = Broodwar->self()->supplyTotal();
}

void BasicAI::onEnd(bool isWinner)
{
	
}

void BasicAI::onFrame() {
 	updateSupply();
	
	if ( analyzed ) {
		onAnalyze();
		analyzed = false;
	}

	Unit* u;
	std::set<Unit*>::const_iterator it, end = Broodwar->self()->getUnits().end();

	for ( it = Broodwar->self()->getUnits().begin(); it != end; ++it ) {
		u = *it;
		 if ( u->getType().isWorker() ) {
			if ( Broodwar->self()->minerals() >= 300 && !natural_base ) {
				u->build( natural->getTilePosition(), UnitTypes::Zerg_Hatchery );
				reserved_minerals += 300;
				natural_base = u;
			} else if ( u->getOrder() == Orders::PlayerGuard ) {
				Unit* closestMineral = NULL;
				int bestDistance = 0;
				std::set<Unit*>::iterator m_it, m_end = Broodwar->getMinerals().end();
				for ( m_it = Broodwar->getMinerals().begin(); m_it != m_end; ++m_it ) {
					if ( closestMineral == NULL || 
						 u->getDistance( *m_it ) < bestDistance ) {
						closestMineral = *m_it;
						bestDistance = u->getDistance(*m_it);
					}
				}
				if ( closestMineral != NULL ) {
					u->rightClick( closestMineral );
				}
			}
		} else if ( u->getType().isResourceDepot() ) {
			if(u->getLarva().size() > 0) {
				if ( availableSupply() <= 2 ) {
					morphLarva( u, UnitTypes::Zerg_Overlord );
				} else {
					morphLarva( u, UnitTypes::Zerg_Drone );
				}
			}
		} else  if ( u->getType() == UnitTypes::Zerg_Overlord ) {
			if ( natural ) {
				u->move( natural->getPosition() );
			}
		}
	}

	drawVisibilityData();
	drawTerrainData();
}

void BasicAI::onUnitMorph(BWAPI::Unit* unit) {
	if ( unit->getType() == UnitTypes::Zerg_Overlord ) {
		overlords_building -= 1;
		overlords_finished += 1;
		Broodwar->printf("Overlord made");
	} else if ( unit->getType() == UnitTypes::Zerg_Egg ) {
		if ( unit->getBuildType() == UnitTypes::Zerg_Overlord ) {
			Broodwar->printf("Egg morphing into Overlord");
			overlords_building += 1;
		}
	} else if( unit->getType() == UnitTypes::Zerg_Hatchery ) {
		reserved_minerals -= 300;
	}
}

void BasicAI::onUnitDestroy(BWAPI::Unit* unit)
{
	if ( unit->getType() == UnitTypes::Zerg_Overlord ) {
		overlords_destroyed += 1;
	}
}

void BasicAI::morphLarva(Unit* base, BWAPI::UnitType unit) {
	int mins = Broodwar->self()->minerals() - reserved_minerals;
	int supply = Broodwar->self()->supplyTotal() - Broodwar->self()->supplyUsed();
	Unit* larva = *base->getLarva().begin();
		
	if ( unit == UnitTypes::Zerg_Overlord ) {
		if ( mins >= 100 ) {
			larva->morph( unit );
		}
	} else if ( unit == UnitTypes::Zerg_Drone ) {
		if ( supply >= 2 && mins >= 50 ) {
			larva->morph( unit );
		}
	}
}

int BasicAI::availableMins() {
	return Broodwar->self()->minerals() - reserved_minerals;
}

int BasicAI::availableSupply() {
	return Broodwar->self()->supplyTotal()
		   - Broodwar->self()->supplyUsed()
		   + 16 * overlords_building
		   + 16 * overlords_finished
		   - 16 * overlords_destroyed;
}

void BasicAI::updateSupply() {
	while ( overlords_finished > 0 &&
		    overlords_destroyed > 0 ) {
		overlords_finished--;
		overlords_destroyed--;
	}

	int supply = Broodwar->self()->supplyTotal();

	if ( supply < past_supply ) {
		overlords_destroyed -= (past_supply - supply) / 16;
	} else if ( supply > past_supply ) {
		overlords_finished -= (supply - past_supply) / 16;
	}

	past_supply = supply;
}

DWORD WINAPI AnalyzeThread()
{
	BWTA::analyze();

	analyzed = true;
	return 0;
}

void BasicAI::onAnalyze() {
	natural = NULL;
	double best_dist;
	BWAPI::Position my_pos = BWTA::getStartLocation(Broodwar->self())->getPosition();

	std::set<BWTA::BaseLocation*>::const_iterator b_it, b_end = BWTA::getBaseLocations().end();
	for(b_it = BWTA::getBaseLocations().begin(); b_it != b_end; ++b_it) {
		if((!natural ||
			(*b_it)->getPosition().getDistance(my_pos) < best_dist) &&
			(*b_it) != BWTA::getStartLocation(Broodwar->self())) {
			best_dist = (*b_it)->getPosition().getDistance(my_pos);
			natural = (*b_it);
		}
	}
}

void BasicAI::drawVisibilityData()
{
	for(int x=0;x<Broodwar->mapWidth();x++)
	{
		for(int y=0;y<Broodwar->mapHeight();y++)
		{
			if (Broodwar->isExplored(x,y))
			{
				if (Broodwar->isVisible(x,y))
					Broodwar->drawDotMap(x*32+16,y*32+16,Colors::Green);
				else
					Broodwar->drawDotMap(x*32+16,y*32+16,Colors::Blue);
			}
			else
				Broodwar->drawDotMap(x*32+16,y*32+16,Colors::Red);
		}
	}
}

void BasicAI::drawTerrainData()
{
	//we will iterate through all the base locations, and draw their outlines.
	for(std::set<BWTA::BaseLocation*>::const_iterator i=BWTA::getBaseLocations().begin();i!=BWTA::getBaseLocations().end();i++)
	{
		TilePosition p=(*i)->getTilePosition();
		Position c=(*i)->getPosition();

		//draw outline of center location
		Broodwar->drawBox(CoordinateType::Map,p.x()*32,p.y()*32,p.x()*32+4*32,p.y()*32+3*32,Colors::Blue,false);

		//draw a circle at each mineral patch
		for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getStaticMinerals().begin();j!=(*i)->getStaticMinerals().end();j++)
		{
			Position q=(*j)->getInitialPosition();
			Broodwar->drawCircle(CoordinateType::Map,q.x(),q.y(),30,Colors::Cyan,false);
		}

		//draw the outlines of vespene geysers
		for(std::set<BWAPI::Unit*>::const_iterator j=(*i)->getGeysers().begin();j!=(*i)->getGeysers().end();j++)
		{
			TilePosition q=(*j)->getInitialTilePosition();
			Broodwar->drawBox(CoordinateType::Map,q.x()*32,q.y()*32,q.x()*32+4*32,q.y()*32+2*32,Colors::Orange,false);
		}

		//if this is an island expansion, draw a yellow circle around the base location
		if ((*i)->isIsland())
			Broodwar->drawCircle(CoordinateType::Map,c.x(),c.y(),80,Colors::Yellow,false);
	}

	//we will iterate through all the regions and draw the polygon outline of it in green.
	for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
	{
		BWTA::Polygon p=(*r)->getPolygon();
		for(int j=0;j<(int)p.size();j++)
		{
			Position point1=p[j];
			Position point2=p[(j+1) % p.size()];
			Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Green);
		}
	}

	//we will visualize the chokepoints with red lines
	for(std::set<BWTA::Region*>::const_iterator r=BWTA::getRegions().begin();r!=BWTA::getRegions().end();r++)
	{
		for(std::set<BWTA::Chokepoint*>::const_iterator c=(*r)->getChokepoints().begin();c!=(*r)->getChokepoints().end();c++)
		{
			Position point1=(*c)->getSides().first;
			Position point2=(*c)->getSides().second;
			Broodwar->drawLine(CoordinateType::Map,point1.x(),point1.y(),point2.x(),point2.y(),Colors::Red);
		}
	}
}

void BasicAI::onSendText(std::string text)
{
}

void BasicAI::onReceiveText(BWAPI::Player* player, std::string text)
{
}

void BasicAI::onPlayerLeft(BWAPI::Player* player)
{
}

void BasicAI::onNukeDetect(BWAPI::Position target)
{
}

void BasicAI::onUnitDiscover(BWAPI::Unit* unit)
{
}

void BasicAI::onUnitEvade(BWAPI::Unit* unit)
{
}

void BasicAI::onUnitShow(BWAPI::Unit* unit)
{
}

void BasicAI::onUnitHide(BWAPI::Unit* unit)
{
}

void BasicAI::onUnitCreate(BWAPI::Unit* unit)
{
}

void BasicAI::onUnitRenegade(BWAPI::Unit* unit)
{
}

void BasicAI::onSaveGame(std::string gameName)
{
}

void BasicAI::onUnitComplete(BWAPI::Unit *unit)
{
}