// This file is part of CaesarIA.
//
// CaesarIA is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// CaesarIA is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CaesarIA.  If not, see <http://www.gnu.org/licenses/>.

#include "objects/construction.hpp"
#include "pathway_helper.hpp"
#include "astarpathfinding.hpp"
#include "gfx/tilemap.hpp"
#include "city/helper.hpp"

using namespace gfx;

Pathway PathwayHelper::create( TilePos startPos, TilePos stopPos,
                               WayType type/*=roadOnly */ )
{
  switch( type )
  {
  case allTerrain: return Pathfinder::getInstance().getPath( startPos, stopPos, Pathfinder::terrainOnly );
  case roadOnly: return Pathfinder::getInstance().getPath( startPos, stopPos, Pathfinder::roadOnly );

  case roadFirst:
  {
    Pathway ret = Pathfinder::getInstance().getPath( startPos, stopPos, Pathfinder::roadOnly );
    if( !ret.isValid() )
    {
      ret = Pathfinder::getInstance().getPath( startPos, stopPos, Pathfinder::roadOnly );
    }

    return ret;
  }
  break;

  case deepWater: return Pathfinder::getInstance().getPath( startPos, stopPos, Pathfinder::deepWaterOnly );
  case water: return Pathfinder::getInstance().getPath( startPos, stopPos, Pathfinder::waterOnly );

  default:
  break;
  }

  return Pathway();
}

Pathway PathwayHelper::create( TilePos startPos, ConstructionPtr construction, PathwayHelper::WayType type)
{
  switch( type )
  {
  case allTerrain: return Pathfinder::getInstance().getPath( startPos, construction->getEnterArea(), Pathfinder::terrainOnly );
  case roadOnly: return Pathfinder::getInstance().getPath( startPos, construction->getAccessRoads(), Pathfinder::roadOnly );

  case roadFirst:
  {
    Pathway ret = Pathfinder::getInstance().getPath( startPos, construction->getAccessRoads(), Pathfinder::roadOnly );

    if( !ret.isValid() )
    {
      ret = Pathfinder::getInstance().getPath( startPos, construction->getEnterArea(), Pathfinder::terrainOnly );
    }

    return ret;
  }
  break;

  default:
  break;
  }

  return Pathway();

}

Pathway PathwayHelper::create(TilePos startPos, TilePos stopPos, const TilePossibleCondition& condition)
{
  Pathfinder::getInstance().setCondition( condition );
  return Pathfinder::getInstance().getPath( startPos, stopPos, Pathfinder::customCondition );
}

DirectRoute PathwayHelper::shortWay(PlayerCityPtr city, TilePos startPos, constants::building::Type buildingType, PathwayHelper::WayType type)
{
  DirectRoute ret;
  city::Helper helper( city );
  ConstructionList constructions = helper.find<Construction>( buildingType );

  foreach( it, constructions )
  {
    Pathway path = create( startPos, *it, type );
    if( path.isValid() )
    {
      if( !ret.way().isValid() )
      {
        ret.second = path;
        ret.first = *it;
      }
      else
      {
        if( ret.way().length() > path.length() )
        {
          ret.second = path;
          ret.first = *it;
        }
      }
    }
  }

  return ret;
}

Pathway PathwayHelper::randomWay( PlayerCityPtr city, TilePos startPos, int walkRadius)
{
  int loopCounter = 0; //loop limiter
  do
  {
    const Tilemap& tmap = city->tilemap();

    TilePos destPos( std::rand() % walkRadius - walkRadius / 2, std::rand() % walkRadius - walkRadius / 2 );
    destPos = (startPos+destPos).fit( TilePos( 0, 0 ), TilePos( tmap.size()-1, tmap.size()-1 ) );

    if( tmap.at( destPos ).isWalkable( true) )
    {
      Pathway pathway = create( startPos, destPos, PathwayHelper::allTerrain );

      if( pathway.isValid() )
      {
        return pathway;
      }
    }
  }
  while( ++loopCounter < 20 );

  return Pathway();
}
