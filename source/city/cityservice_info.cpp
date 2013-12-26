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

#include "cityservice_info.hpp"
#include "city.hpp"
#include "core/safetycast.hpp"
#include "core/position.hpp"
#include "objects/house.hpp"
#include "objects/house_level.hpp"
#include "gfx/tile.hpp"
#include "game/gamedate.hpp"
#include "city/funds.hpp"

class CityParameters
{
public:
  DateTime date;
  int population;
  int funds;
  int tax;
  int taxpayes;

  CityParameters()
  {
    population = 0;
    funds = 0;
    tax = 0;
    taxpayes = 0;
  }
};

class CityServiceInfo::Impl
{
public:
  PlayerCityPtr city;
  DateTime lastDate;
  std::vector< CityParameters > params;
};

CityServicePtr CityServiceInfo::create(PlayerCityPtr city )
{
  CityServicePtr ret( new CityServiceInfo( city ) );
  ret->drop();

  return ret;
}

CityServiceInfo::CityServiceInfo( PlayerCityPtr city )
  : CityService( "info" ), _d( new Impl )
{
  _d->city = city;
  _d->lastDate = GameDate::current();
  _d->params.resize( 12 );
}

void CityServiceInfo::update( const unsigned int time )
{
  if( time % 44 != 1 )
    return;

  if( GameDate::current().getMonth() != _d->lastDate.getMonth() )
  {
    _d->lastDate = GameDate::current();

    _d->params.erase( _d->params.begin() );
    _d->params.push_back( CityParameters() );

    CityParameters& last = _d->params.back();
    last.population = _d->city->getPopulation();
    last.funds = _d->city->getFunds().getValue();    
    last.tax = 0;//_d->city->getFunds().getIssueValue();
    last.taxpayes =  0;//_d->city->getLastMonthTaxpayer();

  }
}