#pragma once

namespace BWTA
{
  class Region;
  class Chokepoint;

  namespace BWTA_Result
  {
    extern std::vector<Region*> regions;
    extern std::set<Chokepoint*> chokepoints;
  };
}
