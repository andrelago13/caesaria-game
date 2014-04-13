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

#include "gamedate.hpp"

namespace {
  unsigned int tickInDay = 25;
}

GameDate& GameDate::instance()
{
  static GameDate inst;
  return inst;
}

unsigned int GameDate::days2ticks(unsigned int days)
{
  return days * tickInDay;
}

void GameDate::timeStep( unsigned int time )
{
  _dayChange = false;
  _weekChange = false;
  _monthChange = false;
  _yearChange = false;

  if( time % tickInDay == 0 )
  {
    DateTime save = _current;
    _current.appendDay();
    _dayChange = true;

    _weekChange = (_current.day() % 7) == 0;
    _monthChange = save.month() != _current.month();
    _yearChange = save.year() != _current.year();
  }  
}

void GameDate::init( const DateTime& date )
{
  _current = date;
}

GameDate::GameDate()
{
  _current = DateTime( -350, 0, 0 );
}
