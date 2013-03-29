#include "ConnectedComponent.h"
#include "functions.h"
namespace BWTA
{
  ConnectedComponent::ConnectedComponent(int id, bool walkable=true) : _id(id),_walkability(walkable),top_left_tile(10000,10000)
  {
  }
  bool ConnectedComponent::isWalkable(void) const
  {
    return this->_walkability;
  }
  void ConnectedComponent::setWalkability(bool walkability)
  {
    this->_walkability=walkability;
  }
  int ConnectedComponent::getID(void) const
  {
    return this->_id;
  }
  PointD ConnectedComponent::top_left() const
  {
    return this->top_left_tile;
  }
  void ConnectedComponent::set_top_left(PointD top_left_tile)
  {
    this->top_left_tile=top_left_tile;
  }
}