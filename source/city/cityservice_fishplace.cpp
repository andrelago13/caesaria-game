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
// Copyright 2012-2013 Dalerank, dalerankn8@gmail.com

#include "cityservice_fishplace.hpp"
#include "objects/construction.hpp"
#include "city/statistic.hpp"
#include "core/safetycast.hpp"
#include "core/utils.hpp"
#include "core/position.hpp"
#include "core/variant_map.hpp"
#include "walker/fish_place.hpp"
#include "game/gamedate.hpp"
#include "cityservice_factory.hpp"
#include "core/common.hpp"
#include "core/tilepos_array.hpp"

namespace city
{

REGISTER_SERVICE_IN_FACTORY(Fishery,fishery)

class Fishery::Impl
{
public:
  unsigned int maxFishPlace;
  int nFailed;

  Locations locations;
  FishPlaceList places;
};

SrvcPtr Fishery::create( PlayerCityPtr city )
{
  SrvcPtr ret( new Fishery( city ) );
  ret->drop();

  return ret;
}

std::string Fishery::defaultName() {  return CAESARIA_STR_EXT(Fishery); }

Fishery::Fishery( PlayerCityPtr city )
  : Srvc( city, Fishery::defaultName() ), _d( new Impl )
{
  _d->nFailed = 0;
  _d->maxFishPlace = 1;
}

void Fishery::timeStep(const unsigned int time )
{  
  if( !game::Date::isMonthChanged() )
    return;

  if( _d->places.empty() )
  {
    _d->places = _city()->statistic().walkers.find<FishPlace>( walker::fishPlace );
  }

  while( _d->places.size() < _d->maxFishPlace )
  {
    FishPlacePtr fishplace = FishPlace::create( _city() );
    TilePos travelingPoint = _d->locations.empty()
                               ? _city()->borderInfo().boatExit
                               : _d->locations.random();

    fishplace->send2city( _city()->borderInfo().boatEntry, travelingPoint );

    if( fishplace->isDeleted() )
    {
      _d->nFailed++;
      return;
    }

    _d->places.push_back( ptr_cast<FishPlace>( fishplace ) );
  }

  utils::eraseIfDeleted( _d->places );
}

bool Fishery::isDeleted() const { return _d->nFailed > 3; }

void Fishery::addLocation(TilePos location)
{
  _d->locations.push_back( location );
}

void Fishery::load(const VariantMap& stream)
{
  Srvc::load( stream );

  VariantMap locations = stream.get( "locations" ).toMap();
  for( auto& location : locations )
  {
    addLocation( location.second.toTilePos() );
  }
}

VariantMap Fishery::save() const
{
  VariantMap ret = Srvc::save();

  VariantMap locationsVm;
  int index = 0;
  for( auto& location : _d->locations )
  {
    locationsVm[ utils::format( 0xff, "fp_%d", index++ ) ] = location;
  }

  ret[ "locations" ] = locationsVm;

  return ret;
}

}//end namespace city
