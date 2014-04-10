#include "TileSet.h"
#include "TileType.h"
#include "../MapData.h"

TileType* TileSet::getTileType(TileID tileID)
{
  if ( BWTA::MapData::TileSet )
    return  (BWTA::MapData::TileSet + ((tileID >> 4 ) & 0x7FF));
  return NULL;
}