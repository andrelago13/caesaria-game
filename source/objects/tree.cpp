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

#include "tree.hpp"
#include "game/resourcegroup.hpp"
#include "gfx/helper.hpp"
#include "city/helper.hpp"
#include "core/foreach.hpp"
#include "gfx/tilemap.hpp"

using namespace gfx;


Tree::Tree()
  : TileOverlay( constants::objects::tree, Size(1) )
{

}

void Tree::timeStep( const unsigned long time )
{
  TileOverlay::timeStep( time );
}

void Tree::initTerrain(Tile& terrain)
{
  terrain.setFlag( Tile::tlTree, true );
}

bool Tree::build(PlayerCityPtr city, const TilePos& pos)
{
  std::string picname = TileHelper::convId2PicName( city->tilemap().at( pos ).originalImgId() );
  setPicture( Picture::load( picname ) );
  return TileOverlay::build( city, pos );
}

void Tree::destroy()
{
  city::Helper helper( _city() );
  TilesArray tiles = helper.getArea( this );
  foreach( it, tiles )
  {
    (*it)->setFlag( Tile::tlTree, false );
  }
}
