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

#include "objects/warehouse.hpp"
#include "mars.hpp"
#include "city/city.hpp"
#include "events/showinfobox.hpp"
#include "game/gamedate.hpp"
#include "core/gettext.hpp"
#include "good/store.hpp"
#include "walker/enemysoldier.hpp"
#include "events/postpone.hpp"
#include "core/saveadapter.hpp"
#include "objects/fort.hpp"
#include "objects/extension.hpp"
#include "city/spirit_of_mars.hpp"
#include "city/statistic.hpp"

using namespace gfx;
using namespace events;

namespace religion
{

namespace rome
{

DivinityPtr Mars::create()
{
  DivinityPtr ret( new Mars() );
  ret->setInternalName( baseDivinityNames[ romeDivMars ] );
  ret->drop();

  return ret;
}

void Mars::updateRelation(float income, PlayerCityPtr city)
{  
  RomeDivinity::updateRelation( income, city );
}

void Mars::_doWrath(PlayerCityPtr city)
{
  GameEventPtr message = ShowInfobox::create( _("##wrath_of_mars_title##"),
                                              _("##wrath_of_mars_text##"),
                                              ShowInfobox::send2scribe,
                                              "god_mars" );
  message->dispatch();

  VariantMap vm = config::load( ":/mars_wrath.model" );
  GameEventPtr barb_attack = PostponeEvent::create( "", vm );
  barb_attack->dispatch();
}

void Mars::_doSmallCurse(PlayerCityPtr city)
{  
  FortList forts = city->statistic().objects.find<Fort>();

  std::string text, title;
  if( !forts.empty() )
  {
    title = "##smallcurse_of_mars_title##";
    text = "##smallcurse_of_mars_text##";
    FortPtr fort = forts.random();
    FortCurseByMars::assignTo( fort, 12 );
  }
  else
  {
    title = "##smallcurse_of_mars_failed_title##";
    text = "##smallcurse_of_mars_failed_text##";
  }

  GameEventPtr message = ShowInfobox::create( _(title),
                                              _(text),
                                              ShowInfobox::send2scribe );

  message->dispatch();
}

void Mars::_doBlessing(PlayerCityPtr city)
{
  GameEventPtr event = ShowInfobox::create( _("##spirit_of_mars_title##"),
                                            _("##spirit_of_mars_text##"),
                                            ShowInfobox::send2scribe );
  event->dispatch();

  city::SrvcPtr spiritOfmars = city::SpiritOfMars::create( city );
  spiritOfmars->attach();
}

}//end namespace rome

}//end namespace religion
