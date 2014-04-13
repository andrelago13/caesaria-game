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

#ifndef _CAESARIA_RANDOMDAMAGE_EVENT_H_INCLUDE_
#define _CAESARIA_RANDOMDAMAGE_EVENT_H_INCLUDE_

#include "event.hpp"

namespace events
{

class RandomDamage : public GameEvent
{
public:
  static GameEventPtr create();
  virtual bool isDeleted() const;

  virtual void load(const VariantMap& stream);
  virtual VariantMap save() const;

protected:
  virtual void _exec(Game &game, unsigned int time);
  virtual bool _mayExec(Game &game, unsigned int time) const;

private:
  RandomDamage();

  int _minPopulation, _maxPopulation;
  VariantMap _events;
  bool _isDeleted;
};

}//end namespace events

#endif //_CAESARIA_RANDOMDAMAGE_EVENT_H_INCLUDE_
