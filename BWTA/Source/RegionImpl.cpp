#include "RegionImpl.h"
namespace BWTA
{
  RegionImpl::RegionImpl(){} // TODO remove after fixing load_data
  RegionImpl::RegionImpl(const Polygon& poly)
	  : _polygon(poly),
	  _maxDistance(0)
  {
	  this->_center = _polygon.getCenter();
  }
  RegionImpl::RegionImpl(const BoostPolygon& boostPoly, const int& scale)
	  : _polygon(Polygon(boostPoly, scale)),
	  _maxDistance(0)
  {
	  this->_center = _polygon.getCenter();
  }
  const Polygon& RegionImpl::getPolygon() const
  {
    return this->_polygon;
  }
  const BWAPI::Position& RegionImpl::getCenter() const
  {
    return this->_center;
  }
  const std::set<Chokepoint*>& RegionImpl::getChokepoints() const
  {
    return this->_chokepoints;
  }
  const std::set<BaseLocation*>& RegionImpl::getBaseLocations() const
  {
    return this->baseLocations;
  }
  bool RegionImpl::isReachable(Region* region) const
  {
    return this->reachableRegions.find(region)!=this->reachableRegions.end();
  }
  const std::set<Region*>& RegionImpl::getReachableRegions() const
  {
    return this->reachableRegions;
  }

  const int RegionImpl::getMaxDistance() const
  {
	  return this->_maxDistance;
  }
}