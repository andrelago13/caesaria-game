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


#ifndef __CAESARIA_PICTURES_ARRAY_H_INCLUDED__
#define __CAESARIA_PICTURES_ARRAY_H_INCLUDED__

#include "picture.hpp"
#include "core/stringarray.hpp"

namespace gfx
{

class Pictures : public std::vector<Picture>
{
public:
  void load( const std::string& rc, int startIndex, int number, int multiplier=1 );
  void load( const StringArray& names );

  Pictures& operator << ( const Picture& pic );
  Pictures& append( const Pictures& pics );
  Pictures& append( const std::string& rc, int index );

  void append( const gfx::Picture& pic, const Point& offset );
  const Picture& valueOrEmpty(unsigned int index ) const;

  StringArray names() const;
};

} //end namespace gfx

#endif //__CAESARIA_PICTURES_ARRAY_H_INCLUDED__
