#include "BaseLocationImpl.h"
namespace BWTA
{
  BaseLocationImpl::BaseLocationImpl(){}
  BaseLocationImpl::BaseLocationImpl(const BWAPI::TilePosition &tp)
  {
    tilePosition=tp;
    position=BWAPI::Position(tp.x()*32+64,tp.y()*32+48);
    island=false;
    region=NULL;
  }

  BWAPI::Position BaseLocationImpl::getPosition() const
  {
    return this->position;
  }
  BWAPI::TilePosition BaseLocationImpl::getTilePosition() const
  {
    return this->tilePosition;
  }
  Region* BaseLocationImpl::getRegion() const
  {
    return this->region;
  }
  int BaseLocationImpl::minerals() const
  {
    int count=0;
    for each (BWAPI::Unit* m in this->staticMinerals)
      count+=m->getResources();
    return count;
  }
  int BaseLocationImpl::gas() const
  {
    int count=0;
    for each (BWAPI::Unit* g in this->geysers)
      count+=g->getResources();
    return count;
  }
  const std::set<BWAPI::Unit*>& BaseLocationImpl::getMinerals()
  {
    std::set<BWAPI::Unit*>::iterator i_next;
    for(std::set<BWAPI::Unit*>::iterator i=this->currentMinerals.begin();i!=this->currentMinerals.end();i=i_next)
    {
      i_next=i;
      i_next++;
      if (!(*i)->exists())
        this->currentMinerals.erase(i);
    }
    for(std::set<BWAPI::Unit*>::iterator i=this->staticMinerals.begin();i!=this->staticMinerals.end();i=i_next)
    {
      i_next=i;
      i_next++;
      if ((*i)->exists())
        this->currentMinerals.insert(*i);
    }
    return this->currentMinerals;
  }
  const std::set<BWAPI::Unit*>& BaseLocationImpl::getStaticMinerals() const
  {
    return this->staticMinerals;
  }
  const std::set<BWAPI::Unit*>& BaseLocationImpl::getGeysers() const
  {
    return this->geysers;
  }

  double BaseLocationImpl::getGroundDistance(BaseLocation* other) const
  {
    std::map<BaseLocation*,double>::const_iterator i=ground_distances.find(other);
    if (i==ground_distances.end())
    {
      return -1;
    }
    return (*i).second;
  }
  double BaseLocationImpl::getAirDistance(BaseLocation* other) const
  {
    std::map<BaseLocation*,double>::const_iterator i=air_distances.find(other);
    if (i==air_distances.end())
    {
      return -1;
    }
    return (*i).second;
  }
  bool BaseLocationImpl::isIsland() const
  {
    return this->island;
  }
  bool BaseLocationImpl::isMineralOnly() const
  {
    return this->geysers.empty();
  }
  bool BaseLocationImpl::isStartLocation() const
  {
    return this->start;
  }
}