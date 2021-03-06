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
//
// Copyright 2012-2014 Dalerank, dalerankn8@gmail.com

#include "terrain_generator.hpp"
#include "gfx/tilemap.hpp"
#include "game/resourcegroup.hpp"
#include "core/logger.hpp"
#include "game/game.hpp"
#include "gfx/helper.hpp"
#include "city/city.hpp"
#include "vfs/file.hpp"
#include "vfs/path.hpp"
#include "core/spline.hpp"
#include "core/direction.hpp"
#include "objects/objects_factory.hpp"
#include "pathway/astarpathfinding.hpp"
#include "gfx/tilearea.hpp"
#include <fstream>
#include <cfloat>

using namespace gfx;

namespace terrain
{

MidpointDisplacement::MidpointDisplacement(int n, int wmult, int hmult, float smoothness, float terrSquare)
{
  n_ = n;
  wmult_ = wmult;
  hmult_ = hmult;
  smoothness_ = smoothness;
  terrainSquare = terrSquare;
  grass_threshold_ = 2.25;
  water_threshold_ = 0.55;
  sand_threshold_ = 0.70;
  deep_water_threshold_ = 0.0001;
  hills_threshold_ = 3.75;
  trees_treshold = 4.0;
  shmountains_threshold = 4.90;
  overall_treshold = 0;
  random_ = Random();
}

int MidpointDisplacement::CoordinatesToVectorIndex(int x, int y)
{
  int i = 0;
  i = x + y * width_;
  return i;
}

std::pair<int, int> MidpointDisplacement::VectorIndexToCoordinates(int i)
{
  int x = 0;
  int y = 0;
  y = i / width_;
  x = i % width_;
  return std::make_pair(x, y);
}

std::vector<int> MidpointDisplacement::map()
{
  int power = pow(2, (float)n_);
  int width = wmult_ * power + 1;
  int height = hmult_ * power + 1;

  width_ = width;
  height_ = height;

  std::vector<float> map;
  std::vector<int> return_map;

  map.resize(width_ * height_);
  return_map.resize(width_ * height_);

  int step = power / 2;
  float sum;
  int count;

  float h = terrainSquare;

  for (int i = 0; i < width_; i += 2 * step) {
    for (int j = 0; j < height_; j+= 2 * step) {
      map[ CoordinatesToVectorIndex(i, j) ] = random_.toFloat(0, 2 * h); //???
    }
  }

  for (int i = 0; i < width_; i++) {
    map[ CoordinatesToVectorIndex(i, 0) ] = random_.toFloat(-3, -2);
    map[ CoordinatesToVectorIndex(i, height_ - 1) ] = random_.toFloat(-2, -1);
  }

  for (int i = 0; i < height_; i++) {
    map[ CoordinatesToVectorIndex(0, i) ] = random_.toFloat(-3, -2);
    map[ CoordinatesToVectorIndex(width_ - 1, i) ] = random_.toFloat(-3, -2);
  }

  map[ CoordinatesToVectorIndex((width_ - 1) / 2, (height_ - 1) / 2) ] = (2 * h) + random_.toFloat(1, 5);
  /*map.at(CoordinatesToVectorIndex(0, 0)) = -3;
map.at(CoordinatesToVectorIndex((width_ - 1), 0)) = -3;
map.at(CoordinatesToVectorIndex(0, (height_ - 1))) = -3;
map.at(CoordinatesToVectorIndex((width_ - 1), (height_ - 1))) = -3;
map.at(CoordinatesToVectorIndex((width_ - 1) / 2, (height_ - 1) / 2)) = 3;*/

  while(step > 0) {

    //Diamond
    for (int x = step; x < width_; x += 2 * step)
    {
      for (int y = step; y < height_; y += 2 * step)
      {
        sum = map[ CoordinatesToVectorIndex(x - step, y - step) ]
            + map[ CoordinatesToVectorIndex(x - step, y + step) ]
            + map[ CoordinatesToVectorIndex(x + step, y - step) ]
            + map[ CoordinatesToVectorIndex(x + step, y + step) ];
        map[ CoordinatesToVectorIndex(x, y) ] = sum / 4 + random_.toFloat(-h, h); //???
      }
    }

    //Square
    for (int x = 0; x < width_; x += step) {
      for (int y = step * (1 - (x / step) % 2); y < height_; y += 2 * step) {
        sum = 0;
        count = 0;
        if(x - step >= 0) {
          sum += map[ CoordinatesToVectorIndex(x - step, y) ];
          count++;
        }
        if(x + step < width) {
          sum += map[ CoordinatesToVectorIndex(x + step, y) ];
          count++;
        }
        if(y - step >= 0) {
          sum += map[ CoordinatesToVectorIndex(x, y - step) ];
          count++;
        }
        if(y + step < height_) {
          sum += map[ CoordinatesToVectorIndex(x, y + step) ];
          count++;
        }
        if(count > 0) {
          map[ CoordinatesToVectorIndex(x, y) ] = sum / count + random_.toFloat(-h, h); //???
        } else {
          map[ CoordinatesToVectorIndex(x, y) ] = 0;
        }
      }
    }
    h /= smoothness_;
    step /= 2;
  }

  float max = FLT_MAX;
  float min = FLT_MIN;

  for (unsigned int i = 0; i < map.size(); i++)
  {
    if(map[ i ] > max)
    {
      map[ i ] = max;
    }

    if(map[ i ] < min)
    {
      map[ i ] = min;
    }
  }

  for (unsigned int i = 0; i < map.size(); i++)
  {
    //std::cout << map.at(i);
    //map.at(i) = (map.at(i) - min) / (max - min);
    //std::cout << map.at(i);
    float value = map[ i ];
    int new_value = unknown;
    if(value < deep_water_threshold_) {      new_value = deepWater;    }
    else if (value < water_threshold_) {      new_value = water;    }
    else if (value < sand_threshold_) {      new_value = coast;    }
    else if (value < grass_threshold_) {      new_value = grass;    }
    else if (value < hills_threshold_) {      new_value = hills;    }
    else if (value < trees_treshold) {      new_value = trees;    }
    else if (value < shmountains_threshold ) {      new_value = shallowMountain;    }
    else {      new_value = ground;    }
   // new_value = (value * 255) + 255;
    return_map[ i ] = new_value;
  }
  return return_map;
}

static void __removeCorners(Game& game )
{
  PlayerCityPtr oCity = game.city();
  Tilemap& oTilemap = oCity->tilemap();
  unsigned int mapSize = oTilemap.size();

  TilePos psAr[8] = { TilePos(0, 1), TilePos(-1, 1), TilePos(-1, 0), TilePos(-1, -1),
                      TilePos(0, -1), TilePos(1, -1), TilePos(1, 0), TilePos(1, 1)};
  //check one water tiles
  for( unsigned int i=0; i < mapSize; i++ )
    for( unsigned int j=0; j < mapSize; j++ )
    {
      TilePos tpos = TilePos( i, j );
      Tile& wtile = oTilemap.at( tpos );
      if( wtile.imgId() == 0 ||
          wtile.getFlag( Tile::tlWater ) || wtile.getFlag( Tile::tlDeepWater ) )
        continue;

      for( int k=0; k < 8; k++ )
      {
        Tile& t = oTilemap.at( tpos + psAr[k] );
        bool isWater = ( t.getFlag( Tile::tlWater ) || t.getFlag( Tile::tlDeepWater ) );
        if( isWater )
        {
          wtile.setFlag( Tile::clearAll, true );
          wtile.setImgId( 0 );
          wtile.setPicture( Picture() );
          break;
        }
      }
    }
}

namespace {
enum { passCheckNorthCoastTiles=1,
       passCheckEastCoastTiles=2,
       passCheckSouthCoastTiles=3,
       passCheckWestCoastTiles=4,
       passCheckSmallCoastTiles=5,
       reserved1=6,
       passCheckInsideCornerTiles=7 };

enum { drN=0x1,drNW=0x80,
       drW=0x8,drSW=0x40,
       drS=0x4,drSE=0x20,
       drE=0x2,drNE=0x10 };

int directions[8] = { drN, drNW, drW, drSW, drS, drSE, drE, drNE };

TilePos psAr[8] = { TilePos(0, 1), TilePos(-1, 1), TilePos(-1, 0), TilePos(-1, -1),
                    TilePos(0, -1), TilePos(1, -1), TilePos(1, 0), TilePos(1, 1)};
}

static void __finalizeMap(Game& game, int pass )
{
  PlayerCityPtr oCity = game.city();
  Tilemap& oTilemap = oCity->tilemap();
  unsigned int mapSize = oTilemap.size();

  //check one water tiles
  for( unsigned int i=0; i < mapSize; i++ )
    for( unsigned int j=0; j < mapSize; j++ )
    {
      TilePos tpos = TilePos( i, j );
      Tile& wtile = oTilemap.at( tpos );
      if( wtile.imgId() > 0 )
        continue;

      int direction = 0;
      for( int k=0; k < 8; k++ )
      {
        Tile& t = oTilemap.at( tpos + psAr[k] );
        bool isWater = !t.getFlag( Tile::tlCoast ) && ( t.getFlag( Tile::tlWater ) || t.getFlag( Tile::tlDeepWater ) );
        if( isWater )
        {
          direction += directions[k];
        }
      }


      int start=0, rnd=4;
      switch( pass )
      {
      case passCheckNorthCoastTiles:
        switch( direction )
        {
        case drN: case drN|drNW: case drNW|drNE|drN: case drNE|drN: case drNE|drNW: start=128; break;
        }
      break;

      case passCheckEastCoastTiles:
        switch( direction )
        {
        case drE: case drNE|drE: case drNE|drSE: case drNE|drSE|drE: case drSE|drE: start=132; break;
        }
      break;

      case passCheckSouthCoastTiles:
        switch( direction )
        {
        case drS: case drSW|drS: case drSE|drS: case drSW|drSE|drS: case drSW|drSE: start=136; break;
        }
      break;

      case passCheckWestCoastTiles:
        switch( direction )
        {
        case drW: case drSW|drNW|drW: case drSW|drW: case drNW|drW: case drSW|drNW: start=140; break;
        }
      break;

      case passCheckSmallCoastTiles:
        switch( direction )
        {
        case drE|drS|drSE:
        case drE|drNE|drSE|drS:
        case drE|drSW|drSE|drS:
        case drE|drSW|drSE|drS|drNE:
        case drE|drSW|drSE|drNE:
        case drE|drSW|drNE: case drNE|drSE|drSW|drS:
        case drSE|drNE|drS: case drNE|drS|drE: case drSW|drE:
        case drSW|drS|drE: case drNE|drS: case drNE|drSW|drS:
        case drNE|drSW|drE|drS:
        case drE|drS: case drNE|drSE|drSW: case drSW|drSE|drE: start = 171; rnd=0; break;

        case drNW|drSW|drW|drN: case drNW|drW|drN: case drNW|drNE|drW|drN:
        case drNW|drNE|drW: case 0xd9: case 0x58: case 0x19:
        case 0xd8: case 0xd1: case 0x49: case 0x18: case 0xc1: case 0x41:
        case 0x51: case 0x09: case 0x59: case 0xd0: start=173; rnd=0; break;

        case 0x13: case 0xb1: case 0x23: case 0x93: case 0x33: case 0xb3: case 0x83:
        case 0x31: case 0xa3: case 0x21: case 0x82: case 0xa1: case 0xa2: case 0x92:
        case 0xb2: case 0x03: case 0xb0: start=170; rnd=0; break;

        case 0xa0: start=168; rnd=0; break;
        case 0x50: start=169; rnd=0; break;

        case 0xcc: case 0x4c: case 0x6c: case 0xec:
        case 0xc4: case 0x68: case 0xe4: case 0xe8: case 0xa8: case 0x84:
        case 0x28: case 0xac: case 0xa4: case 0x0c: case 0x8c: case 0x2c:
        case 0xe0:  start = 172; rnd=0; break;
        }
      break;

      case 6:
      break;

      case passCheckInsideCornerTiles:
        switch( direction )
        {
        case drNE: start=PicID::coastNE; break;
        case drSE: start=PicID::coastSE; break;
        case 0x40: start=152; break;
        case 0x80: start=156; break;
        }
      break;

      case 8:
        switch( direction )
        {
        case 0x6e: case 0xed: case 0x9b: case 0xcd: case 0xff: case 0xee:
        case 0xdf: case 0x05: case 0x37: case 0x77: case 0xfb: case 0x9a:
        case 0xeb: case 0xa5: case 0xf9: case 0xb7: case 0xbf: case 0x1b:
        case 0x35: case 0xe5: case 0x95: case 0x17: case 0x7d: case 0xe7:
        case 0xad: case 0x71: case 0xdc: case 0xdb: case 0xa6: case 0xb9:
        case 0xa7: case 0x27: case 0xdd: case 0xf3: case 0x7e: case 0x75:
        case 0xd3: case 0xc5: case 0x55: case 0xfe: case 0xbb: case 0x7c:
        case 0x5a: case 0x53: case 0x45: case 0xce: case 0xef: case 0x7f:
        case 0x8d: case 0x1d: case 0xf5: case 0xf7: case 0xf1:
        case 0x97: case 0x2e: case 0xc3: case 0x5c: case 0x0a: case 0x69:
        case 0xb5: case 0x25: case 0x8a: case 0x3b: case 0xe6: case 0x6a:
        case 0xcb: case 0x3f: case 0xaa: case 0x57: case 0xfd: case 0xd5:
        case 0x7a: case 0x85: case 0xfc: case 0xca: case 0x8b: case 0x9f:
        case 0x4f: case 0xde: case 0x9d: case 0xe9: case 0x6f: case 0xd7:
        case 0xae: case 0xaf: case 0xf8: case 0x3c: case 0xe2: case 0xea:
        case 0x96: case 0x3a: case 0xcf: case 0xbd: case 0xbe: case 0x94:
        case 0x73: case 0x4a: case 0xc6: case 0x5d: case 0x2a: case 0xa9:
        case 0x6d: case 0xf4: case 0xda: case 0x5e: case 0xfa: case 0x7b:
        case 0x79: case 0x65: case 0xc7: case 0xe1: case 0x4d: case 0x2b:
        case 0xab: case 0x0e: case 0x87: case 0x1a: case 0x67: case 0x3d:
        case 0xf6: case 0x1f: case 0xd6: case 0xb4: case 0x8f: case 0x9c:
        case 0x48: case 0x5f: case 0x5b: case 0xd4: case 0xf0: case 0x0f:
        case 0x15: case 0x0b: case 0xb8: case 0x07: case 0x1c: case 0x61:
        case 0x8e: case 0x86: case 0x2d: case 0x4b: case 0xe3: case 0xc2:
        case 0xba: case 0xf2: case 0x1e: case 0x39: case 0x38: case 0xd2:
        case 0x78: case 0x0d: case 0x6b: case 0x3e: case 0x63: case 0x29:
        case 0x47: case 0x21: case 0xbc: case 0x2f: case 0xb6: case 0x43:
        case 0x9e: case 0x4e: start=120; rnd=0; break;
        }
      break;

      case 9:
        switch( direction )
        {
        case 0:
        {
          Picture pic( ResourceGroup::land1a, 62 + math::random( 56 ) );
          wtile.setPicture( pic );
          //wtile.setOriginalImgId( TileHelper::convPicName2Id( pic.name() ) );
          wtile.setImgId( direction );
        }
        break;
        }
      break;

      case 0xff:
        wtile.setImgId( direction );
      break;
      }

      if( start > 0 )
      {
        if( rnd > 0 )
          rnd = math::random( 3 );

        wtile.setFlag( Tile::tlWater, true );
        wtile.setFlag( Tile::tlCoast, true );
        Picture pic( ResourceGroup::land1a, start + rnd );
        wtile.setPicture( pic );
        wtile.setImgId( imgid::fromResource( pic.name() ) );
      }
    }
}

class TerrainGeneratorHelper
{
public:
  int mapSize;
  void canBuildRoad( const Tile* tile, bool& ret )
  {
    ret = tile->isWalkable( true ) || tile->getFlag( Tile::tlTree );
  }

  void canBuildRiver( const Tile* tile, bool& ret )
  {
    ret = (  tile->getFlag( Tile::tlWater ) || tile->getFlag( Tile::tlGrass ) || tile->getFlag( Tile::tlTree ) );
  }

  void needBuildRoad( const Tile* tile, bool& ret )
  {
    ret = tile->isWalkable( true ) || tile->getFlag( Tile::tlTree );

    if( tile->getFlag( Tile::tlWater ) )
    {
      ret = false;
      return;
    }

    if( !ret )
    {        
      const int border = 10;
      if( tile->getFlag( Tile::tlRock )
          && (tile->i() < border || tile->j() < border || (mapSize - tile->i() < border) || (mapSize - tile->j() < border)) )
      {
        ret = true;
      }
    }
  }

  Pathway findWay( Tilemap& oTilemap, TilePos startPos, TilePos endPos )
  {
    mapSize = oTilemap.size();
    Pathfinder& pathfinder = Pathfinder::instance();
    pathfinder.setCondition( makeDelegate( this, &TerrainGeneratorHelper::needBuildRoad ) );

    return pathfinder.getPath( startPos, endPos, Pathfinder::customCondition | Pathfinder::fourDirection );
  }

  Pathway findWay(Tilemap& oTilemap, int startSide, int endSide )
  {
    Pathfinder& pathfinder = Pathfinder::instance();
    pathfinder.setCondition( makeDelegate( this, &TerrainGeneratorHelper::canBuildRoad ) );

    TilesArray tiles = sideTiles( startSide, oTilemap );
    tiles = tiles.walkables( true );

    if( tiles.empty() )
      return Pathway();

    TilesArray otherTiles = sideTiles( endSide, oTilemap );
    otherTiles = otherTiles.walkables( true );

    return findWay( tiles, otherTiles );
  }

  Pathway findWay( const TilesArray& startTiles, const TilesArray& arrivedTiles )
  {
    if( arrivedTiles.empty() || startTiles.empty() )
      return Pathway();

    Pathfinder& pathfinder = Pathfinder::instance();
    unsigned int mainLimiter = 10;
    unsigned int stTileLimiter = startTiles.size() > mainLimiter
                                   ? startTiles.size() / mainLimiter : 1;
    for( unsigned int tileIndex = 0; tileIndex < startTiles.size(); tileIndex += stTileLimiter )
    {
      Tile* rtile = startTiles[ tileIndex ];

      unsigned int arvTileLimiter = arrivedTiles.size() > mainLimiter
                                      ? arrivedTiles.size() / mainLimiter : 1;
      for( unsigned int arvTileIndex=0; arvTileIndex < arrivedTiles.size(); arvTileIndex += arvTileLimiter )
      {
        Tile* endTile = arrivedTiles[ arvTileIndex ];

        Pathway way = pathfinder.getPath( *rtile, *endTile, Pathfinder::customCondition | Pathfinder::fourDirection );

        if( way.isValid() )
          return way;
      }
    }

    return Pathway();
  }

  TilesArray sideTiles( int side, Tilemap& oTilemap )
  {
    int mapSize = oTilemap.size();
    switch( side % 4 )
    {
    case 0: return oTilemap.area( TilePos( 0, 0), TilePos( 0, mapSize-1 ) );
    case 1: return oTilemap.area( TilePos( 0, mapSize-1), TilePos( mapSize-1, mapSize-1 ) );
    case 2: return oTilemap.area( TilePos( mapSize-1, mapSize-1), TilePos( mapSize-1, 0 ) );
    case 3: return oTilemap.area( TilePos( mapSize-1, 0), TilePos( 0, 0 ) );
    }

    return TilesArray();
  }
};

static void __createRivers(Game& game )
{
  PlayerCityPtr oCity = game.city();
  Tilemap& oTilemap = oCity->tilemap();

  TerrainGeneratorHelper tgHelper;
  Pathfinder& pathfinder = Pathfinder::instance();
  pathfinder.setCondition( makeDelegate( &tgHelper, &TerrainGeneratorHelper::canBuildRiver ) );

  TilesArray terrainTiles = oTilemap.allTiles();
  for( TilesArray::iterator it=terrainTiles.begin(); it != terrainTiles.end(); )
  {
    Tile* tile = *it;
    if( tile->isWalkable( true ) || tile->getFlag( Tile::tlTree ) ) { ++it; }
    else { it = terrainTiles.erase( it ); }
  }

  int riverCount = 0;
  for( int tryCount=0; tryCount < 20;  tryCount++ )
  {
    if( riverCount++ > oTilemap.size() / 60 )
      break;

    Tile* centertile = terrainTiles.random();
    Pathway way;
    for( int range=0; range < 99; range++ )
    {
      TilesArray perimeter = oTilemap.rect( range, centertile->pos() );
      for( auto currentTile : perimeter )
      {
        if( currentTile->getFlag( Tile::tlWater ) )
        {
          way = pathfinder.getPath( *centertile, *currentTile, Pathfinder::customCondition | Pathfinder::fourDirection );
          if( way.isValid() )
          {
            break;
          }
        }
      }

      if( way.isValid() )
        break;
    }

    if( way.isValid() )
    {
      TilesArray wayTiles = way.allTiles();
      for( auto tile : wayTiles )
      {
        OverlayPtr overlay = TileOverlayFactory::instance().create( object::river );

        tile->setPicture( Picture::getInvalid() );
        tile->setImgId( 0 );

        bool isWater = tile->getFlag( Tile::tlWater );

        TilePos epos = tile->epos();

        city::AreaInfo info( oCity, epos );
        overlay->build( info );
        oCity->addOverlay( overlay );

        if( isWater )
          break;
      }
    }
  }
}

static void __createRoad(Game& game )
{
  PlayerCityPtr oCity = game.city();
  Tilemap& oTilemap = oCity->tilemap();

  TerrainGeneratorHelper tgHelper;

  Pathway way;
  for( int side=0; side < 2; side++ )
  {
    way = tgHelper.findWay( oTilemap, side, side + 2 );

    if( !way.isValid() )
    {
      way = tgHelper.findWay( oTilemap, side, side + 1 );
    }

    if( !way.isValid() )
    {
      way = tgHelper.findWay( oTilemap, side, side + 3 );
    }

    if( way.isValid() )
      break;
  }

  BorderInfo borderInfo = oCity->borderInfo();
  TilesArray borderTiles = oTilemap.border();

  if( !way.isValid() )
  {
    TilesArray eterrain = borderTiles.terrains();
    TilesArray sterrain;
    for( unsigned int k=eterrain.size()/2; k < eterrain.size(); k++ )
      sterrain.push_back( eterrain[ k ] );

    eterrain.resize( eterrain.size() / 2 );

    for( int tryCount=0; tryCount < 10; tryCount++ )
    {
      borderInfo.roadEntry = sterrain.random()->pos();
      borderInfo.roadExit = eterrain.random()->pos();

      way = tgHelper.findWay( oTilemap, borderInfo.roadEntry, borderInfo.roadExit );
      if( way.isValid() )
        break;

      TilesArray around = oTilemap.area( 3, borderInfo.roadEntry );
      for( auto tile : around )
        sterrain.remove( tile->pos() );

      around = oTilemap.area( 3, borderInfo.roadExit );
      for( auto tile : around )
        eterrain.remove( tile->pos() );
    }
  }

  if( way.isValid() )
  {
    TilesArray wayTiles = way.allTiles();

    for( auto tile : wayTiles )
    {
      OverlayPtr overlay = TileOverlayFactory::instance().create( object::road );

      Picture pic( ResourceGroup::land1a, PicID::grassPic + math::random( PicID::grassPicsNumber-1 ) );
      tile->setPicture( pic );
      tile->setImgId( imgid::fromResource( pic.name() ) );

      city::AreaInfo info( oCity, tile->epos() );
      overlay->build( info );
      oCity->addOverlay( overlay );
    }

    borderInfo.roadEntry = way.startPos();
    borderInfo.roadExit = way.stopPos();
  }  
  else
  {
    TilesArray terrain = borderTiles.terrains();

    borderInfo.roadEntry = terrain.random()->pos();
    borderInfo.roadExit = terrain.random()->pos();       
  }

  TilesArray water = borderTiles.waters();
  borderInfo.boatEntry = water.random()->pos();
  borderInfo.boatExit = water.random()->pos();

  oCity->setBorderInfo( borderInfo );
}

TilesArray& addTileIfValid( TilesArray& tiles, int i, int j, Tilemap& tmap )
{
  Tile& t = tmap.at( i, j );
  if( tilemap::isValidLocation( t.epos() ) )
    tiles.push_back( &t );

  return tiles;
}

TilesArray getTmapEllipse(int ox, int oy, int width, int height, double angle, Tilemap& tmap)
{
  TilesArray tiles;
  double c = cos( angle * math::DEGTORAD );
  double s = sin( angle * math::DEGTORAD );
  for(int y=-height; y<=height; y++)
  {
      for(int x=-width; x<=width; x++)
      {
          if(x*x*height*height+y*y*width*width <= height*height*width*width)
          {
            int xt = c*x + s*y;
            int yt = -s*x + c*y;
            addTileIfValid( tiles, ox+xt, oy+yt, tmap );
          }
      }
  }

  return tiles;
}

TilesArray getTmapCircle(int rx, int ry, int r, Tilemap& tmap )
{
  TilesArray ret;
  int x,y;
  int rsqr = pow(r,2);
  for(x=rx-r; x<=rx+r; x++)
    for(y=ry-r; y<=ry+r; y++)
      if( ( pow(x-rx,2)+pow(y-ry,2) )< rsqr)
        addTileIfValid( ret, x, y, tmap);

  return ret;
}

void __createMeadows( Game& game )
{
  PlayerCityPtr oCity = game.city();
  Tilemap& oTilemap = oCity->tilemap();

  TilesArray tiles = oTilemap.allTiles();
  int fieldSize = math::max<int>( tiles.size() / 7000.f, 20 );
  for( int k=0; k < fieldSize; k++ )
  {
    Tile* tile = tiles.random();

    //TilesArray meadows = getTmapCircle( tile->i(), tile->j(), math::random( fieldSize-1 ), oTilemap );
    TilesArray meadows = getTmapEllipse( tile->i(), tile->j(), fieldSize, fieldSize/2, math::random(180), oTilemap );
    meadows = meadows.terrains();

    for( auto tile : meadows )
      tile->setFlag( Tile::tlMeadow, true );
  }
}

void Generator::create(Game& game, int n2size, float smooth, float terrainSq)
{
  MidpointDisplacement diamond_square = MidpointDisplacement(n2size, 8, 8, smooth, terrainSq);
  std::vector<int> map = diamond_square.map();

  /*vfs::NFile nfile = vfs::NFile::open( vfs::Path( "test.ter" ), vfs::Entity::fmWrite );
  nfile.write( map.data(), map.size() * 4 );
  nfile.flush();*/

  /*std::ifstream rfile( "test.ter", std::ios::binary );
  if( rfile.is_open() )
  {
    std::streampos begin,end;
    begin = rfile.tellg();
    rfile.seekg (0, std::ios::end);
    end = rfile.tellg();

    rfile.seekg (0, std::ios::beg);
    map.resize( (end-begin) / 4 );

    rfile.read( (char*)map.data(), end-begin );

    rfile.close();
  }*/

  PlayerCityPtr oCity = game.city();
  Tilemap& oTilemap = oCity->tilemap();
  unsigned int mapSize = diamond_square.width();

  oCity->resize( mapSize );

  Logger::warning( "W:%d H:%d", diamond_square.width(), diamond_square.height() );

  for( unsigned int index = 0; index < map.size(); index++)
  {
    int j = index / (diamond_square.width());
    int i = index % (diamond_square.width());

    Tile& tile = oTilemap.at(i, j);
    //tile.setPicture( TileHelper::convId2PicName( pGraphicGrid.data()[index] ) );
    //tile.setOriginalImgId( pGraphicGrid.data()[index] );

    //TileHelper::decode( tile, pTerrainGrid.data()[index] );
    NColor color;
    tile.setFlag( Tile::clearAll, true );

    switch( map[ index ] )
    {
      case MidpointDisplacement::unknown:
      {
        color = NColor( 255, 255, 0, 255);
        tile.setFlag( Tile::tlDeepWater, true );
        Picture pic( ResourceGroup::land1a, 120 );
        tile.setPicture( pic );
        tile.setImgId( imgid::fromResource( pic.name() ) );
      }
      break;

      case MidpointDisplacement::deepWater:
      {
        color = NColor( 255, 7, 93, 192);
        tile.setFlag( Tile::tlDeepWater, true );
        Picture pic( ResourceGroup::land1a, 120 );
        tile.setPicture( pic );
        tile.setImgId( imgid::fromResource( pic.name() ) );
      }
      break;

      case MidpointDisplacement::water:
      {
        color = NColor( 255, 74, 157, 251);
        tile.setFlag( Tile::tlWater, true );
        Picture pic( ResourceGroup::land1a, 120 );
        tile.setPicture( pic );
        tile.setImgId( imgid::fromResource( pic.name() ) );
      }
      break;

      case MidpointDisplacement::coast: {
        color = NColor( 255, 255, 238, 178);
      }
      break;

      case MidpointDisplacement::grass:
      {
        color = NColor( 255, 7, 192, 53);
        Picture pic( ResourceGroup::land1a, 62 + math::random( 56 ) );
        tile.setPicture( pic );
        tile.setImgId( imgid::fromResource( pic.name() ) );
      }
      break;

      case MidpointDisplacement::hills:
      case MidpointDisplacement::trees:
      {
        color = NColor( 255, 32, 139, 58);
        int start=30, rnd=31;
        if( math::random( 10 ) > 6 )
        {
          start = 10;
          rnd = 6;
        }

        if( tile.i() == 0 || tile.i() == ((int)mapSize - 1) || tile.j() == 0 || tile.j() == ((int)mapSize - 1) )
        {
          start = 62;
          rnd = 56;
        }
        else
        {
          tile.setFlag( Tile::tlTree, true );
        }

        Picture land = MetaDataHolder::randomPicture( object::terrain, Size(1) );
        tile.setPicture( land );

        Picture tree( ResourceGroup::land1a, start + math::random( rnd ) );
        tile.setImgId( imgid::fromResource( tree.name() ) );

        OverlayPtr overlay = TileOverlayFactory::instance().create( object::tree );
        if( overlay != NULL )
        {
          city::AreaInfo info( oCity, tile.pos() );
          overlay->build( info );
          oCity->addOverlay( overlay );
        }
      }
      break;

      case MidpointDisplacement::ground:
      {
        color = NColor( 255, 129, 141, 132);
        Picture pic( ResourceGroup::land1a, 62 + math::random( 56 ) );
        //Picture::load( ResourceGroup::land1a, 230 + math::random( 59 ) );
        //tile.setFlag( Tile::tlRock, true );
        tile.setPicture( pic );
        //tile.setHeight( 1 );
        tile.setImgId( imgid::fromResource( pic.name() ) );
      }
      break;

      case MidpointDisplacement::shallowMountain:
      {
        color = NColor( 255, 147, 188, 157 );
        Picture pic( ResourceGroup::land1a, 290 + math::random( 6 ) );
        tile.setFlag( Tile::tlRock, true );
        tile.setPicture( pic );
        tile.setImgId( imgid::fromResource( pic.name() ) );
      }
      break;

      case 9: {
        color = DefaultColors::white;
      }
      break;
    }
  }

  __removeCorners( game );

  __finalizeMap( game, passCheckNorthCoastTiles );
  __finalizeMap( game, passCheckEastCoastTiles );
  __finalizeMap( game, passCheckSouthCoastTiles );
  __finalizeMap( game, passCheckWestCoastTiles );
  __finalizeMap( game, passCheckSmallCoastTiles );
  //__finalizeMap( game, 6 );
  __finalizeMap( game, passCheckInsideCornerTiles );
  __finalizeMap( game, 8 );
  __finalizeMap( game, 9 );
  __createMeadows( game );
  __finalizeMap( game, 0xff );

  //update pathfinder map
  Pathfinder::instance().update( oTilemap );

  __createRivers( game );
  __createRoad( game ); 
}

void Generator::create(Game &game, const Generator::Params &params)
{
  create( game, params.n2size, params.smooth, params.terrainSq );
}

void Generator::Params::load(const VariantMap& vm)
{
  n2size = vm.get( "size", 5 );
  smooth = vm.get( "smooth", 2.6 );
  terrainSq = vm.get( "terrain", 4 );
}

}//end namespace terrain
