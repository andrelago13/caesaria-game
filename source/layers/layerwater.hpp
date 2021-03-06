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
// Copyright 2012-2015 Dalerank, dalerankn8@gmail.com

#ifndef __CAESARIA_LAYERWATER_H_INCLUDED__
#define __CAESARIA_LAYERWATER_H_INCLUDED__

#include "layer.hpp"
#include "gfx/city_renderer.hpp"

namespace citylayer
{

class Water : public Layer
{
public:
  virtual int type() const;
  virtual void drawTile( gfx::Engine& engine, gfx::Tile& tile, const Point& offset );
  virtual void drawPass( gfx::Engine &engine, gfx::Tile& tile, const Point& offset, gfx::Renderer::Pass pass);
  virtual void drawWalkerOverlap(gfx::Engine &engine, gfx::Tile &tile, const Point& offset, const int depth);

  static LayerPtr create( gfx::Camera& camera, PlayerCityPtr city );
  virtual void handleEvent(NEvent& event);

private:
  void _drawLandTile( gfx::Engine &engine, gfx::Tile &tile, const Point &offset, const Size &areaSize);

  class Impl;
  ScopedPtr<Impl> _d;

  Water( gfx::Camera& camera, PlayerCityPtr city );
};

}//end namespace citylayer

#endif //__CAESARIA_LAYERWATER_H_INCLUDED__
