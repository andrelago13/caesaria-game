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

#include "layers/constants.hpp"
#include "menu.hpp"
#include "texturedbutton.hpp"
#include "gfx/picture.hpp"
#include "core/color.hpp"
#include "game/resourcegroup.hpp"
#include "core/event.hpp"
#include "buildmenu.hpp"
#include "environment.hpp"
#include "gfx/decorator.hpp"
#include "widgetpositionanimator.hpp"
#include "label.hpp"
#include "core/gettext.hpp"
#include "game/minimap_colours.hpp"
#include "gfx/city_renderer.hpp"
#include "gfx/tile.hpp"
#include "gfx/engine.hpp"
#include "overlays_menu.hpp"
#include "core/foreach.hpp"
#include "core/utils.hpp"
#include "image.hpp"
#include "core/variant_map.hpp"
#include "core/logger.hpp"
#include "objects/constants.hpp"
#include "city/city.hpp"
#include "extented_date_info.hpp"
#include "core/saveadapter.hpp"
#include "core/osystem.hpp"
#include "events/playsound.hpp"

using namespace constants;
using namespace gfx;
using namespace city;
using namespace events;

namespace gui
{

static const int REMOVE_TOOL_ID = 0xff00 + 1;
static const int MAXIMIZE_ID = REMOVE_TOOL_ID + 1;

struct Menu::Config
{
  typedef enum { smallMenu=0, bigMenu } MenuType;
  Picture background, bottom;
  int width;
  float scale;
  bool fitToScreen;

  Config( Widget* parent, bool fit, const std::string& name, MenuType mode )
   : fitToScreen( fit )
  {
    VariantMap config = config::load( name );

    VARIANT_LOAD_PICTURE(bottom, config)
    VARIANT_LOAD_PICTURE(background, config)

    if( !background.isValid() )
      background.load( ResourceGroup::panelBackground, mode == smallMenu ? 16 : 17 );

    if( !bottom.isValid() )
      bottom.load( ResourceGroup::panelBackground, mode == smallMenu ? 21 : 20 );

    scale = 1;
    if( fitToScreen )
      scale = (parent->height() * 0.9f) / (float)background.height();
    width = background.width() * scale;
  }
};

class Menu::Impl
{
public:
  struct {
    Pictures pics;
    Rects rects;
    Batch batch;

    void add( Picture pic, const Rect& r )
    {
      pics.push_back( pic );
      rects.push_back( r );

      batch.destroy();
      batch.load( pics, rects );
    }

    void update( const Point& move )
    {
      //no move, do nothing
      if( move == Point( 0, 0) )
        return;

      for( auto&& r : rects )
        r += move;

      batch.destroy();
      batch.load( pics, rects );
    }
  } bg;

  Widget* lastPressed;
  PushButton* menuButton;
  PushButton* minimizeButton;
  PushButton* senateButton;
  PushButton* empireButton;
  PushButton* missionButton;
  PushButton* northButton;
  PushButton* rotateLeftButton;
  PushButton* rotateRightButton;
  PushButton* messageButton;
  PushButton* disasterButton;
  PushButton* houseButton;
  PushButton* waterButton;
  PushButton* clearButton;
  PushButton* roadButton;
  PushButton* administrationButton;
  PushButton* entertainmentButton;
  PushButton* educationButton;
  PushButton* templeButton;
  PushButton* commerceButton;
  PushButton* securityButton;
  PushButton* healthButton;
  PushButton* engineerButton;
  PushButton* cancelButton;
  PushButton* overlaysButton;
  Image* middleLabel;
  OverlaysMenu* overlaysMenu; 
  float koeff;
  PlayerCityPtr city;

  struct {
    Signal1<int> onCreateConstruction;
    Signal0<> onRemoveTool;
    Signal0<> onHide;
  } signal;

  void initActionButton(PushButton* btn, const Point& pos,
                        bool pushBtn=true);
  void playSound(Widget* widget);
  void updateBuildingOptions();
};

Signal1<int>& Menu::onCreateConstruction(){  return _d->signal.onCreateConstruction;}
Signal0<>& Menu::onRemoveTool(){  return _d->signal.onRemoveTool;}

class MenuButton : public TexturedButton
{
public:
  MenuButton( Widget* parent, const Point& pos, int id, int midIconId, int startPic, bool pushBtn )
    : TexturedButton( parent, pos, Size( 39, 26 ), id, startPic )
  {
    _midIconId = midIconId;
    setIsPushButton( pushBtn );
  }

  int midPicId() const { return _midIconId; }
  void setMidPicId( int id ) { _midIconId = id; }
  void setSound( const std::string& name ) { addProperty( "sound", name ); }
private:
  int _midIconId;
};

Menu::Menu(Widget* parent, int id, const Rect& rectangle , PlayerCityPtr city)
  : Widget( parent, id, rectangle ), _d( new Impl )
{
  setupUI( ":/gui/shortmenu.gui" );
  _d->city = city;
  _d->lastPressed = 0;
  _d->overlaysMenu = 0;
}

void Menu::_updateButtons()
{
  const bool haveSubMenu = true;
  _d->minimizeButton = _addButton( ResourceMenu::maximizeBtn, false, 0, MAXIMIZE_ID,
                                   !haveSubMenu, ResourceMenu::emptyMidPicId, "show_bigpanel",
                                   Rect( Point( 6, 4 ), Size( 31, 20 ) ) );

  _d->houseButton = _addButton( ResourceMenu::houseBtnPicId, true, 0, object::house,
                                !haveSubMenu, ResourceMenu::houseMidPicId, "housing" );

  _d->clearButton = _addButton( 131, true, 1, REMOVE_TOOL_ID,
                                !haveSubMenu, ResourceMenu::clearMidPicId, "clear_land" );

  _d->roadButton = _addButton( 135, true, 2, object::road, !haveSubMenu, ResourceMenu::roadMidPicId, "road" );
  _d->waterButton = _addButton( 127, true, 3, development::water, haveSubMenu, ResourceMenu::waterMidPicId, "water" );
  _d->healthButton = _addButton( 163, true, 4, development::health, haveSubMenu, ResourceMenu::healthMidPicId, "health" );
  _d->templeButton = _addButton( 151, true, 5, development::religion, haveSubMenu, ResourceMenu::religionMidPicId, "temples" );
  _d->educationButton = _addButton( 147, true, 6, development::education, haveSubMenu, ResourceMenu::educationMidPicId, "education" );

  _d->entertainmentButton = _addButton( 143, true, 7, development::entertainment, haveSubMenu,
                                        ResourceMenu::entertainmentMidPicId, "entertainment" );

  _d->administrationButton = _addButton( 139, true, 8, development::administration, haveSubMenu,
                                         ResourceMenu::administrationMidPicId, "administration" );

  _d->engineerButton = _addButton( 167, true, 9, development::engineering, haveSubMenu,
                                   ResourceMenu::engineerMidPicId, "engineering" );

  _d->securityButton = _addButton( 159, true, 10, development::security, haveSubMenu,
                                   ResourceMenu::securityMidPicId, "security" );

  _d->commerceButton = _addButton( 155, true, 11, development::commerce, haveSubMenu,
                                   ResourceMenu::comerceMidPicId, "comerce" );

  CONNECT( _d->minimizeButton, onClicked(), this, Menu::minimize );
}

void Menu::_initialize( const Config& config )
{
  Impl& d = *_d.data();
  d.koeff = config.scale;
  d.bg.add( config.background, Rect( Point(0, 0), config.background.size() * config.scale ) );

  unsigned int y = config.background.height() * config.scale;
  while( y < parent()->height() )
  {
    d.bg.add( config.bottom, Rect( 0, y, config.width, y + config.bottom.height() * config.scale ) );
    y += config.bottom.height() * config.scale - 5;
  }
}

void Menu::_setChildGeometry( Widget* w, const Rect& r)
{
  if( !w )
    return;

  w->setGeometry( r * _d->koeff );
}

void Menu::_updateBuildOptions()
{
  _d->updateBuildingOptions();
}

PushButton* Menu::_addButton( int startPic, bool pushBtn, int yMul, 
                              int id, bool haveSubmenu, int midPic,
                              const std::string& ident, const Rect& rect )
{
  Point offset( 1, 32 );
  int dy = 35;

  MenuButton* ret = new MenuButton( this, Point( 0, 0 ), -1, -1, startPic, pushBtn );
  ret->setID( id | ( haveSubmenu ? BuildMenu::subMenuCreateIdHigh : 0 ) );
  Point temp = offset + Point( 0, dy * yMul );
  if( _d->koeff != 1 )
  {
    temp.setX( ceil( temp.x() * _d->koeff) );
    temp.setY( temp.y() * _d->koeff );
    ret->setWidth( ceil( ret->width() * _d->koeff ) );
    ret->setHeight( ceil( ret->height() * _d->koeff ) );
  }
  ret->setPosition( temp );
  ret->setTooltipText( _( "##extm_"+ident+"_tlp##" ) );
  ret->setSound( "extm_" + ident );
  ret->setMidPicId( midPic );

  if( rect.width() > 0 )
    _setChildGeometry( ret, rect );

  return ret;
}

/* here will be helper functions for minimap generation */
void Menu::draw(gfx::Engine& painter)
{
  if( !visible() )
    return;

  if( _d->bg.batch.valid() )
    painter.draw( _d->bg.batch, &absoluteClippingRectRef() );
  else
    painter.draw( _d->bg.pics, absoluteRect().lefttop(), &absoluteClippingRectRef() );
    
  Widget::draw( painter );
}

void Menu::setPosition(const Point& relativePosition)
{
  Point oldPos = lefttop();
  Widget::setPosition( relativePosition );
  _d->bg.update( relativePosition - oldPos );
}

bool Menu::onEvent(const NEvent& event)
{
  if( event.EventType == sEventGui && event.gui.type == guiButtonClicked )
  {
    if( !event.gui.caller )
        return false;

    int id = event.gui.caller->ID();
    if( id == object::house || id == object::road )
    {
      _d->lastPressed = event.gui.caller;
      _createBuildMenu( -1, this );
      emit _d->signal.onCreateConstruction( id );
    }
    else if( id == REMOVE_TOOL_ID )
    {
      _d->lastPressed = event.gui.caller;
      _createBuildMenu( -1, this );
      emit _d->signal.onRemoveTool();
    }
    else
    {
      if( _d->lastPressed != event.gui.caller )
      {
        if( event.gui.caller->parent() == this )
            _d->lastPressed = event.gui.caller;

        if( PushButton* btn = safety_cast< PushButton* >( event.gui.caller ) )
        {
          int id = btn->ID();
          if( id & BuildMenu::subMenuCreateIdHigh )
          {
            _createBuildMenu( id & 0xff, event.gui.caller );
          }
          else
          {
            emit _d->signal.onCreateConstruction( id );
            _createBuildMenu( -1, this );
            setFocus();
          }
        }
      }
    }

    unselectAll();
    if( PushButton* btn = safety_cast< PushButton* >( _d->lastPressed ) )
    {
      btn->setPressed( true && btn->isPushButton() );
    }
    return true;
  }

  if( event.EventType == sEventGui && event.gui.type == guiElementFocusLost )
  {
    if( findChildren<BuildMenu*>().empty() )
    {
      unselectAll();
      _d->lastPressed = 0;
    }
  }

  if( event.EventType == sEventMouse )
  {
    switch( event.mouse.type )
    {
    case mouseRbtnRelease:
      _createBuildMenu( -1, this );
      unselectAll();
      _d->lastPressed = 0;
    return true;

    case mouseLbtnPressed:
    case mouseLbtnRelease:
    {
      //lock movement for tilemap
      if( findChildren<BuildMenu*>().size() > 0 )
        return true;
    }
    break;

    default: break;
    }
  }

  return Widget::onEvent( event );
}

Menu* Menu::create(Widget* parent, int id, PlayerCityPtr city, bool fitToScreen )
{
  Config config( parent, fitToScreen, ":/menu.model", Config::smallMenu );

  Menu* ret = new Menu( parent, id, Rect( 0, 0, config.width, parent->height() ), city );

  ret->_initialize( config  );
  ret->_updateButtons();
  ret->_updateBuildOptions();

  CONNECT( city, onChangeBuildingOptions(), ret, Menu::_updateBuildOptions );

  return ret;
}

void Menu::minimize()
{
  _d->lastPressed = 0;
  _createBuildMenu( -1, this );
  Point stopPos = lefttop() + Point( width(), 0 );
  auto animator = new PositionAnimator( this, WidgetAnimator::removeSelf, stopPos, 300 );
  CONNECT( animator, onFinish(), &_d->signal.onHide, Signal0<>::_emit );

  auto event = PlaySound::create( "panel", 3, 100 );
  event->dispatch();
}

void Menu::maximize()
{
  Point stopPos = lefttop() - Point( width(), 0 );
  show();
  new PositionAnimator( this, WidgetAnimator::showParent | WidgetAnimator::removeSelf, stopPos, 300 );

  auto event = PlaySound::create( "panel", 3, 100 );
  event->dispatch();
}

bool Menu::unselectAll()
{
  bool anyPressed = false;
  auto buttons = children().select<PushButton>();
  for( auto btn : buttons )
  {
    anyPressed |= btn->isPressed();
    btn->setPressed( false );
  }

  return anyPressed;
}

void Menu::_createBuildMenu( int type, Widget* parent )
{
   auto menus = findChildren<BuildMenu*>();
   for( auto m : menus ) { m->deleteLater(); }

   BuildMenu* buildMenu = BuildMenu::create( (development::Branch)type, this,
                                             _d->city->getOption( PlayerCity::c3gameplay ) );

   if( buildMenu != NULL )
   {
     buildMenu->setNotClipped( true );
     buildMenu->setBuildOptions( _d->city->buildOptions() );
     buildMenu->initialize();

     int y = math::clamp< int >( parent->screenTop() - screenTop(), 0, _environment->rootWidget()->height() - buildMenu->height() );
     buildMenu->setPosition( Point( -(int)buildMenu->width() - 5, y ) );
   }
}

Signal0<>& Menu::onHide() { return _d->signal.onHide; }

void Menu::Impl::initActionButton(PushButton* btn, const Point& pos, bool pushBtn )
{
  btn->setPosition( pos * koeff );
  btn->setIsPushButton( pushBtn );
  CONNECT( btn, onClickedEx(), this, Impl::playSound );
}

void Menu::Impl::playSound( Widget* widget )
{
  std::string sound = widget->getProperty( "sound" ).toString();
  int index = 1;
  if( sound.empty() )
  {
    sound = "panel";
    index = math::random( 2 ) + 1;
  }

  auto event = PlaySound::create( sound, index, 100 );
  event->dispatch();
}

void Menu::Impl::updateBuildingOptions()
{
  const development::Options& options = city->buildOptions();
  waterButton->setEnabled( options.isGroupAvailable( development::water ));
  administrationButton->setEnabled( options.isGroupAvailable( development::administration ));
  entertainmentButton->setEnabled( options.isGroupAvailable( development::entertainment ));
  educationButton->setEnabled( options.isGroupAvailable( development::education ));
  templeButton->setEnabled( options.isGroupAvailable( development::religion ));
  commerceButton->setEnabled( options.isGroupAvailable( development::commerce ));
  securityButton->setEnabled( options.isGroupAvailable( development::security ));
  healthButton->setEnabled( options.isGroupAvailable( development::health ));
  engineerButton->setEnabled( options.isGroupAvailable( development::engineering ));
}

ExtentMenu* ExtentMenu::create(Widget* parent, int id, PlayerCityPtr city , bool fitToScreen)
{
  fitToScreen = true;
  Config config( parent, fitToScreen, ":/extmenu.model", Config::bigMenu );

  ExtentMenu* ret = new ExtentMenu( parent, id, Rect( 0, 0, config.width, parent->height() ), city );
  ret->setID( Hash( CAESARIA_STR_A(ExtentMenu)) );

  ret->_initialize( config );
  ret->_updateButtons();
  ret->_updateBuildOptions();

  CONNECT( city, onChangeBuildingOptions(), ret, ExtentMenu::_updateBuildOptions );

  return ret;
}

ExtentMenu::ExtentMenu(Widget* p, int id, const Rect& rectangle, PlayerCityPtr city )
    : Menu( p, id, rectangle, city )
{
  setupUI( ":/gui/fullmenu.gui" );
  _d->city = city;
}

void ExtentMenu::_updateButtons()
{
  Menu::_updateButtons();

  _d->minimizeButton->deleteLater();
  _d->minimizeButton = _addButton( 97, false, 0, MAXIMIZE_ID, false, ResourceMenu::emptyMidPicId,
                                   "hide_bigpanel" );
  _setChildGeometry( _d->minimizeButton, Rect( Point( 127, 5 ), Size( 31, 20 ) ) );

  CONNECT( _d->minimizeButton, onClicked(), this, ExtentMenu::minimize );

  _d->initActionButton( _d->houseButton, Point( 13, 277 ), false );
  _d->initActionButton( _d->clearButton, Point( 63, 277 ), false );
  _d->initActionButton( _d->roadButton, Point( 113, 277 ), false );
  _d->initActionButton( _d->waterButton, Point( 13, 313 ) );
  _d->initActionButton( _d->healthButton, Point( 63, 313 ) );
  _d->initActionButton( _d->templeButton, Point( 113, 313 ) );
  _d->initActionButton( _d->educationButton, Point(13, 349 ));
  _d->initActionButton( _d->entertainmentButton, Point(63, 349 ) );
  _d->initActionButton( _d->administrationButton, Point( 113, 349) );
  _d->initActionButton( _d->engineerButton, Point( 13, 385 ) );
  _d->initActionButton( _d->securityButton, Point( 63, 385 ) );
  _d->initActionButton( _d->commerceButton, Point( 113, 385) );

  //header
  _d->senateButton = _addButton( 79, false, 0, -1, false, -1, "senate" );
  _setChildGeometry( _d->senateButton, Rect( Point( 7, 155 ), Size( 71, 23 ) ) );

  _d->empireButton = _addButton( 82, false, 0, -1, false, -1, "empire" );
  _setChildGeometry( _d->empireButton, Rect( Point( 84, 155 ), Size( 71, 23 ) ) );

  _d->missionButton = _addButton( 85, false, 0, -1, false, -1, "mission" );
  _setChildGeometry( _d->missionButton, Rect( Point( 7, 184 ), Size( 33, 22 ) ) );

  _d->northButton = _addButton( 88, false, 0, -1, false, -1, "reorient_map_to_north" );
  _setChildGeometry( _d->northButton, Rect( Point( 46, 184 ), Size( 33, 22 ) ) );

  _d->rotateLeftButton = _addButton( 91, false, 0, -1, false, -1, "rotate_map_counter_clockwise" );
  _setChildGeometry( _d->rotateLeftButton, Rect( Point( 84, 184 ), Size( 33, 22 ) ) );

  _d->rotateRightButton = _addButton( 94, false, 0, -1, false, -1, "rotate_map_clockwise" ) ;
  _setChildGeometry( _d->rotateRightButton, Rect( Point( 123, 184 ), Size( 33, 22 ) ) );

  _d->cancelButton = _addButton( 171, false, 0, -1, false, -1, "cancel" );
  _setChildGeometry( _d->cancelButton, Rect( Point( 13, 421 ), Size( 39, 22 ) ) );
  _d->cancelButton->setEnabled( false );

  _d->messageButton = _addButton( 115, false, 0, -1, false, -1, "message" );
  _setChildGeometry( _d->messageButton, Rect( Point( 63, 421 ), Size( 39, 22 ) ) );

  _d->disasterButton = _addButton( 119, false, 0, -1, false, -1, "troubles" );
  _setChildGeometry( _d->disasterButton, Rect( Point( 113, 421 ), Size( 39, 22 ) ) );
  _d->disasterButton->setEnabled( false );

  _d->middleLabel = new Image( this, Rect( 0, 0, 1, 1 ), Picture(), Image::fit );
  _setChildGeometry( _d->middleLabel, Rect( Point( 7, 216 ), Size( 148, 52 )) );
  _d->middleLabel->setPicture( Picture( ResourceGroup::menuMiddleIcons, ResourceMenu::emptyMidPicId ) );

  _d->overlaysMenu = new OverlaysMenu( parent(), Rect( 0, 0, 160, 1 ), -1 );
  _d->overlaysMenu->hide();

  _d->overlaysButton = new PushButton( this, Rect( 0, 0, 1, 1 ), _("##ovrm_text##") );
  _setChildGeometry( _d->overlaysButton, Rect( 4, 3, 122, 28 ) );
  _d->overlaysButton->setTooltipText( _("##select_city_layer##") );

  CONNECT( _d->overlaysButton, onClicked(), this, ExtentMenu::toggleOverlayMenuVisible );
  CONNECT( _d->overlaysMenu, onSelectOverlayType(), this, ExtentMenu::changeOverlay );
}

bool ExtentMenu::onEvent(const NEvent& event)
{
  if( event.EventType == sEventGui && event.gui.type == guiButtonClicked )
  {
    MenuButton* btn = safety_cast< MenuButton* >( event.gui.caller );
    if( btn )
    {
      int picId = btn->midPicId() > 0 ? btn->midPicId() : ResourceMenu::emptyMidPicId;
      _d->middleLabel->setPicture( Picture( ResourceGroup::menuMiddleIcons, picId ) );
    }
  }

  return Menu::onEvent( event );
}

void ExtentMenu::draw(Engine& painter )
{
  if( !visible() )
    return;

  Menu::draw( painter );
}

void ExtentMenu::toggleOverlayMenuVisible()
{
  _d->overlaysMenu->setPosition( Point( screenLeft() - 170, 74 ) );
  _d->overlaysMenu->setVisible( !_d->overlaysMenu->visible() );
}

void ExtentMenu::resolveUndoChange(bool enabled)
{
  _d->cancelButton->setEnabled( enabled );
}

void ExtentMenu::changeOverlay(int ovType)
{
  std::string layerName = citylayer::Helper::prettyName( (citylayer::Type)ovType );
  _d->overlaysButton->setText( _( layerName ) );
}

void ExtentMenu::showInfo(int type)
{
  int hash = Hash(CAESARIA_STR_A(ExtentedDateInfo));
  ExtentedDateInfo* window = safety_cast<ExtentedDateInfo*>( findChild( hash ) );
  if( !window )
  {
    window = new ExtentedDateInfo( this, Rect( Point(), size() ), hash );
  }
  else
  {
    window->deleteLater();
  }
}

void ExtentMenu::setAlarmEnabled( bool enabled )
{
  if( enabled )
  {
    auto event = events::PlaySound::create( "extm_alarm", 1, 100, audio::effects );
    event->dispatch();
  }

  _d->disasterButton->setEnabled( enabled );
}

Signal1<int>& ExtentMenu::onSelectOverlayType() {  return _d->overlaysMenu->onSelectOverlayType(); }
Signal0<>& ExtentMenu::onEmpireMapShow(){  return _d->empireButton->onClicked(); }
Signal0<>& ExtentMenu::onAdvisorsWindowShow(){  return _d->senateButton->onClicked(); }
Signal0<>& ExtentMenu::onSwitchAlarm(){  return _d->disasterButton->onClicked(); }
Signal0<>& ExtentMenu::onMessagesShow()  { return _d->messageButton->onClicked(); }
Signal0<>& ExtentMenu::onRotateRight() { return _d->rotateRightButton->onClicked(); }
Signal0<>& ExtentMenu::onRotateLeft() { return _d->rotateLeftButton->onClicked(); }
Signal0<>& ExtentMenu::onUndo() { return _d->cancelButton->onClicked(); }
Signal0<>& ExtentMenu::onMissionTargetsWindowShow(){  return _d->missionButton->onClicked(); }

}//end namespace gui
