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

#include "immigrant.hpp"
#include "core/position.hpp"
#include "core/safetycast.hpp"
#include "pathway/pathway_helper.hpp"
#include "objects/house.hpp"
#include "gfx/tile.hpp"
#include "core/variant.hpp"
#include "city/statistic.hpp"
#include "gfx/helper.hpp"
#include "pathway/path_finding.hpp"
#include "gfx/tilemap.hpp"
#include "name_generator.hpp"
#include "objects/constants.hpp"
#include "city/migration.hpp"
#include "game/resourcegroup.hpp"
#include "corpse.hpp"
#include "city/states.hpp"
#include "core/variant_map.hpp"
#include "core/variant_list.hpp"
#include "gfx/cart_animation.hpp"
#include "walkers_factory.hpp"

using namespace gfx;

REGISTER_CLASS_IN_WALKERFACTORY(walker::emigrant, Emigrant)

namespace  {
CAESARIA_LITERALCONST(peoples)
const int maxFailedWayCount = 10;
const int populationOverVillage=300;
const int minDesirability4settle=-10;
}

class Emigrant::Impl
{
public:
  CartAnimation cart;
  CitizenGroup peoples;
  int failedWayCount;
  TilePos housePosLock;
  bool cartBackward;
  bool leaveCity;
  float stamina;

public:
  void mayWalk( const Tile* tile, bool& ret )
  {
    HousePtr f = tile->overlay<House>();
    ret = ( tile->isWalkable( true ) || f.isValid() );
  }  
};

Emigrant::Emigrant(PlayerCityPtr city )
  : Human( city ), _d( new Impl )
{
  _setType( walker::emigrant );

  setName( NameGenerator::rand( NameGenerator::male ) );
  _d->stamina = math::random( 80 ) + 20;
  _d->failedWayCount = 0;
  _d->leaveCity = false;
  _d->cartBackward = true;
  _d->housePosLock = gfx::tilemap::invalidLocation();
}

void Emigrant::_lockHouse( HousePtr house )
{
  if( _d->housePosLock.i() >= 0 )
  {
    auto oldHouse = _map().overlay( _d->housePosLock ).as<House>();
    if( oldHouse.isValid() )
    {
      _d->housePosLock = gfx::tilemap::invalidLocation();
      oldHouse->setState( pr::settleLock, 0 );
    }
  }

  if( house.isValid() )
  {
    _d->housePosLock = house->pos();
    house->setState( pr::settleLock, tile::hash( _d->housePosLock ) );
  }
}

HousePtr Emigrant::_findBlankHouse()
{
  HousePtr blankHouse;

  TilePos offset( 5, 5 );
  HouseList houses = _city()->statistic().objects.find<House>( object::house, pos() - offset, pos() + offset );

  _checkHouses( houses );

  if( houses.empty() )
  {
    houses = _city()->statistic().houses.find();
    _checkHouses( houses );
  }

  if( houses.size() > 0 )
  {
    blankHouse = houses.random();
  }

  return blankHouse;
}

Pathway Emigrant::_findSomeWay( TilePos startPoint )
{
  HousePtr house = _findBlankHouse();  
  _d->failedWayCount++;

  Pathway pathway;
  if( house.isValid() )
  {    
    pathway = PathwayHelper::create( startPoint, house->pos(), PathwayHelper::roadFirst  );

    if( !pathway.isValid() )
    {
      pathway = PathwayHelper::create( startPoint, ptr_cast<Construction>(house),
                                       PathwayHelper::roadFirst  );
    }

    if( pathway.isValid() )
    {
      _lockHouse( house );
    }
  }

  if( !pathway.isValid() || _d->failedWayCount > maxFailedWayCount )
  {    
    pathway = PathwayHelper::create( startPoint,
                                     _city()->borderInfo().roadExit,
                                     PathwayHelper::allTerrain );
  }

  return pathway;
}

void Emigrant::_reachedPathway()
{
  bool gooutCity = true;
  Walker::_reachedPathway();

  if( pos() == _city()->borderInfo().roadExit )
  {
    auto migration = _city()->statistic().services.find<city::Migration>();

    if( migration.isValid() )
    {
      migration->citizenLeaveCity( this );
    }

    deleteLater();
    return;
  }

  auto house = _city()->getOverlay( pos() ).as<House>();
  if( house.isValid() )
  {
    _append2house( house );
    gooutCity = (_d->peoples.count() > 0);
  }
  else
  {
    if( _checkNearestHouse() )
    {
      return;
    }
  }

  if( gooutCity )
  {
    Pathway way = _findSomeWay( pos() );
    if( way.isValid() )
    {
      _updatePathway( way );
      go();
    }
    else
    {
      die();
    }
  }
  else
  {
    deleteLater();
  }
}

void Emigrant::_append2house( HousePtr house )
{
  int freeRoom = house->capacity() - house->habitants().count();
  if( freeRoom > 0 )
  {
    house->addHabitants( _d->peoples );
    _lockHouse( HousePtr() );
  }
}

bool Emigrant::_checkNearestHouse()
{
  for( int k=1; k < 3; k++ )
  {
    TilePos offset( k, k );
    HouseList houses = _city()->statistic().objects.find<House>( object::house, pos()-offset, pos() + offset );

    std::map< int, HousePtr > vacantRoomPriority;
    for( auto house : houses )
    {
      unsigned int freeRoom = house->capacity() - house->habitants().count();
      vacantRoomPriority[ 1000 - freeRoom ] = house;
    }

    for( auto& item : vacantRoomPriority )  //have destination
    {
      int freeRoom = item.second->capacity() - item.second->habitants().count();
      if( freeRoom > 0 )
      {
        Pathway pathway = PathwayHelper::create( pos(), item.second->pos(), makeDelegate( _d.data(), &Impl::mayWalk ) );

        _updatePathway( pathway );
        go();
        return true;
      }
    }
  }

  return false;
}

void Emigrant::_brokePathway(TilePos p)
{
  if( _nextTile().overlay().is<House>() )
  {
    return;
  }

  _reachedPathway();

  if( isDeleted() )
    return;

  Pathway way = _findSomeWay( pos() );
  if( way.isValid() )
  {
    setPathway( way );
    go();
  }
  else
  {
    die();
  }
}

void Emigrant::_noWay()
{
  Pathway someway = PathwayHelper::randomWay( _city(), pos(), 5 );
  if( !someway.isValid() )
  {
    _d->failedWayCount++;
    if( _d->failedWayCount > maxFailedWayCount )
    {
      die();
    }
  }
  else
  {
    _d->failedWayCount = 0;
    setPathway( someway );
    go();
  }
}

bool Emigrant::_isCartBackward() const { return _d->cartBackward; }

void Emigrant::_splitHouseFreeRoom(HouseList& moreRooms, HouseList& lessRooms )
{
  lessRooms.clear();

  unsigned int myPeoples = _d->peoples.count();

  HouseList::iterator itHouse = moreRooms.begin();
  while( itHouse != moreRooms.end() )
  {
    HousePtr house = *itHouse;
    unsigned int freeRoom = house->capacity() - house->habitants().count();
    if( freeRoom > 0 )
    {
      if( freeRoom > myPeoples )
      {
        ++itHouse;
      }
      else
      {
        lessRooms.push_back( *itHouse );
        itHouse = moreRooms.erase( itHouse );
      }
    }
    else
    {
      itHouse = moreRooms.erase( itHouse );
    }
  }
}

void Emigrant::_findFinestHouses(HouseList& hlist)
{
  HouseList::iterator itHouse = hlist.begin();
  bool bigcity = _city()->states().population > populationOverVillage;
  unsigned int houseLockId = tile::hash( _d->housePosLock );

  while( itHouse != hlist.end() )
  {
    HousePtr house = *itHouse;
    bool haveRoad = !house->roadside().empty();
    bool haveVacantRoom = (house->habitants().count() < house->capacity());
    bool normalDesirability = true;
    if( bigcity )
    {
      normalDesirability = (house->tile().param( Tile::pDesirability ) > minDesirability4settle);
    }

    unsigned int settleLockId = house->state( pr::settleLock );
    if( settleLockId == houseLockId )
    {
      hlist.clear();
      hlist.push_back( house );
      return;
    }

    bool freeForSettle = ( 0 == settleLockId);

    if( freeForSettle && haveRoad && haveVacantRoom && normalDesirability) { ++itHouse; }
    else { itHouse = hlist.erase( itHouse ); }
  }
}

void Emigrant::_checkHouses(HouseList &hlist)
{
  if( hlist.empty() )
    return;

  HouseList lessRoomHouses;
  _splitHouseFreeRoom( hlist, lessRoomHouses );

  if( !hlist.empty() )
  {
    _findFinestHouses( hlist );
  }

  if( !hlist.empty() )
    return;

  if( !lessRoomHouses.empty() )
  {
    _findFinestHouses( lessRoomHouses );
    hlist = lessRoomHouses;
  }
}

EmigrantPtr Emigrant::create(PlayerCityPtr city )
{
  EmigrantPtr ret( new Emigrant( city ) );
  ret->drop(); //delete automatically
  return ret;
}

EmigrantPtr Emigrant::send2city( PlayerCityPtr city, const CitizenGroup& peoples,
                                 const Tile& startTile, std::string thinks )
{
  if( peoples.count() > 0 )
  {
    EmigrantPtr im = Emigrant::create( city );
    im->setPeoples( peoples );
    im->send2city( startTile );
    im->setThinks( thinks );
    return im;
  }

  return EmigrantPtr();
}

bool Emigrant::send2city( const Tile& startTile )
{    
  setPos( startTile.pos() );
  Pathway way = _findSomeWay( startTile.pos() );

  if( way.isValid() )
  {
    setPathway( way );
    attach();
    return true;
  }
  else
  {
    deleteLater();
    return false;
  }
}

void Emigrant::leaveCity( const Tile& tile )
{
  setPos( tile.pos() );
  Pathway pathway = PathwayHelper::create( tile.pos(),
                                           _city()->borderInfo().roadExit,
                                           PathwayHelper::allTerrain );

  if( !pathway.isValid() )
  {
    die();
    return;
  }

  attach();
  _d->leaveCity = true;
  setPathway( pathway );
  go();
}


Emigrant::~Emigrant()
{
  _lockHouse( HousePtr() );
}

void Emigrant::_setCart( const CartAnimation& anim ){  _d->cart = anim; }
CartAnimation& Emigrant::_cart(){  return _d->cart; }
const CitizenGroup& Emigrant::peoples() const{  return _d->peoples;}
void Emigrant::setPeoples( const CitizenGroup& peoples ){  _d->peoples = peoples;}

void Emigrant::timeStep(const unsigned long time)
{
  Walker::timeStep( time );

  switch( action() )
  {
  case Walker::acMove:
    _d->stamina = math::clamp( _d->stamina-0.5f, 0.f, 100.f );
    if( _d->stamina == 0 )
    {
      _setAction( Walker::acNone );
    }
  break;

  case Walker::acNone:
    _d->stamina = math::clamp( _d->stamina+1, 0.f, 100.f );
    if( _d->stamina >= 100 )
    {
      if( !_d->leaveCity )
      {
        Pathway way = _findSomeWay( pos() );
        if( way.isValid() )
        {
          _updatePathway( way );
        }
      }

      go();
    }
  break;

  default:
  break;
  }

  _d->cart.update( time );
}

TilePos Emigrant::places(Walker::Place type) const
{
  switch( type )
  {
  case plDestination: return _d->housePosLock;
  default: break;
  }

  return Human::places( type );
}

void Emigrant::save( VariantMap& stream ) const
{
  Walker::save( stream );
  stream[ literals::peoples ] = _d->peoples.save();
  VARIANT_SAVE_ANY_D( stream, _d, stamina )
}

void Emigrant::load( const VariantMap& stream )
{
  Walker::load( stream );
  _d->peoples.load( stream.get( literals::peoples ).toList() );
  VARIANT_LOAD_ANY_D( _d, stamina, stream )
}

bool Emigrant::die()
{
  bool created = Walker::die();

  if( !created )
  {
    Corpse::create( _city(), pos(), ResourceGroup::citizen2, 1007, 1014 );
    return true;
  }

  _lockHouse( HousePtr() );

  return created;
}

void Emigrant::initialize(const VariantMap &options)
{
  Human::initialize( options );

  _d->cartBackward = options.get( "cartBackward", _d->cartBackward );
}
