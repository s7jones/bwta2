#pragma once
#include "functions.h"
namespace BWTA
{
  class ConnectedComponent
  {
  public:
    ConnectedComponent(int id, bool walkable);
    bool isWalkable(void) const;
    void setWalkability(bool walkability);
    int getID(void) const;
    PointD top_left() const;
    void set_top_left(PointD top_left_tile);
    private:
    PointD top_left_tile;
    bool _walkability;
    int _id;
  };
}