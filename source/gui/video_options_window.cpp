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

#include "video_options_window.hpp"
#include "pushbutton.hpp"
#include "core/event.hpp"
#include "listbox.hpp"
#include "core/utils.hpp"
#include "dialogbox.hpp"
#include "core/gettext.hpp"
#include "environment.hpp"
#include "core/foreach.hpp"
#include "core/logger.hpp"
#include "gameautopause.hpp"
#include "widget_helper.hpp"
#include "widgetescapecloser.hpp"

namespace gui
{

namespace dialog
{

class VideoOptions::Impl
{
public:
  GameAutoPause locker;
  PushButton* btnSwitchMode;
  bool fullScreen;
  bool haveChanges;

  struct {
    Signal1<Size> onScreenSizeChange;
    Signal1<bool> onFullScreeChange;
    Signal0<> onClose;
  } signal;
};

VideoOptions::VideoOptions(Widget* parent, gfx::Engine::Modes modes, bool fullscreen )
  : Window( parent, Rect( 0, 0, 1, 1 ), "" ), _d( new Impl )
{
  _d->locker.activate();
  setupUI( ":/gui/videooptions.gui" );

  setPosition( Point( parent->width() - width(), parent->height() - height() ) / 2 );
  GET_DWIDGET_FROM_UI( _d, btnSwitchMode )

  _d->fullScreen = fullscreen;
  _d->haveChanges = false;

  INIT_WIDGET_FROM_UI( ListBox*, lbxModes )
  if( lbxModes )
  {
    std::string modeStr;

    for( auto& mode : modes )
    {
      modeStr = utils::format( 0xff, "%dx%d", mode.width(), mode.height() );
      ListBoxItem& item = lbxModes->addItem( modeStr );
      item.setTag( (mode.width() << 16) + mode.height());
      item.setTextAlignment( align::center, align::center );
    }
  }

  _update();
  WidgetEscapeCloser::insertTo( this );

  INIT_WIDGET_FROM_UI( PushButton*, btnOk )
  if( btnOk ) btnOk->setFocus();
}

VideoOptions::~VideoOptions( void ){}

bool VideoOptions::onEvent(const NEvent& event)
{
  if( event.EventType == sEventGui )
  {
    switch( event.gui.type )
    {
    case guiButtonClicked:
    {
      switch( event.gui.caller->ID() )
      {
      case 1:
      {
        _d->fullScreen = !_d->fullScreen;
        emit _d->signal.onFullScreeChange( _d->fullScreen );
        _d->haveChanges = true;
        _update();
      }
      break;

      default:
        if( _d->haveChanges )
        {
          dialog::Information( ui(), "", _("##need_restart_for_apply_changes##"));
        }
        deleteLater();
      break;
      }

      return true;
    }
    break;

    case guiListboxChanged:
    {
      _d->haveChanges = true;
      ListBox* lbx = safety_cast< ListBox* >( event.gui.caller );

      int tag = lbx->selectedItem().tag();

      emit _d->signal.onScreenSizeChange( Size( (tag>>16) & 0xffff, tag & 0xffff ) );
    }
    break;

    default: break;
    }
  }

  return Widget::onEvent( event );
}

Signal1<Size>& VideoOptions::onSreenSizeChange() {  return _d->signal.onScreenSizeChange; }
Signal1<bool>& VideoOptions::onFullScreenChange(){  return _d->signal.onFullScreeChange; }
Signal0<>& VideoOptions::onClose(){  return _d->signal.onClose; }

void VideoOptions::_update()
{
  if( _d->btnSwitchMode )
  {
    _d->btnSwitchMode->setText( _d->fullScreen ? _("##fullscreen_on##") : _("##fullscreen_off##") );
  }
}

}//end namespace dialog

}//end namespace gui
