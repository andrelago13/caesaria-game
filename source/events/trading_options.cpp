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

#include "trading_options.hpp"
#include "core/utils.hpp"
#include "game/game.hpp"
#include "world/empire.hpp"
#include "good/helper.hpp"
#include "world/computer_city.hpp"
#include "factory.hpp"

namespace events
{

REGISTER_EVENT_IN_FACTORY(ChangeTradingOptions, "trading_options")

GameEventPtr ChangeTradingOptions::create()
{
  GameEventPtr ret( new ChangeTradingOptions() );
  ret->drop();

  return ret;
}

void ChangeTradingOptions::load(const VariantMap& stream)
{
  _options = stream;
}

void ChangeTradingOptions::_exec(Game& game, unsigned int)
{
  VariantMap citiesVm = _options.get( "cities" ).toMap();
  for( auto& it : citiesVm )
  {
    world::CityPtr cityp = game.empire()->findCity( it.first );
    if( cityp.isNull() )
    {
      unsigned int tradeDelayMnt = it.second.toMap().get( "delay_trade" );
      if( tradeDelayMnt > 0 )
      {
        cityp->delayTrade( tradeDelayMnt );
      }

      world::ComputerCityPtr ccity = cityp.as<world::ComputerCity>();
      if( ccity.isValid() )
      {
        ccity->changeTradeOptions( it.second.toMap() );
      }
    }
  }

  VariantMap goodsVm = _options.get( "goods" ).toMap();
  for( auto& it : goodsVm )
  {
    good::Product gtype = good::Helper::getType( it.first );
    if( gtype != good::none )
    {
      VariantMap goodInfo = it.second.toMap();
      bool relative = goodInfo.get( "relative", false );
      world::PriceInfo prices;
      prices.buy = goodInfo.get( "buy" );
      prices.sell = goodInfo.get( "sell" );
      if( relative ) { game.empire()->changePrice( gtype, prices ); }
      else { game.empire()->setPrice( gtype, prices ); }
    }
  }
}

bool ChangeTradingOptions::_mayExec(Game&, unsigned int) const{  return true; }

}
