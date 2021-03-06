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

#include "advisor_emperor_window.hpp"
#include "gfx/decorator.hpp"
#include "core/gettext.hpp"
#include "pushbutton.hpp"
#include "objects/construction.hpp"
#include "label.hpp"
#include "core/logger.hpp"
#include "game/resourcegroup.hpp"
#include "core/utils.hpp"
#include "gfx/engine.hpp"
#include "groupbox.hpp"
#include "listbox.hpp"
#include "listboxitem.hpp"
#include "core/logger.hpp"
#include "city/city.hpp"
#include "city/request.hpp"
#include "core/foreach.hpp"
#include "good/helper.hpp"
#include "game/gamedate.hpp"
#include "gameautopause.hpp"
#include "city/statistic.hpp"
#include "city/victoryconditions.hpp"
#include "city/requestdispatcher.hpp"
#include "game/player.hpp"
#include "dialogbox.hpp"
#include "events/fundissue.hpp"
#include "change_salary_window.hpp"
#include "game/funds.hpp"
#include "world/empire.hpp"
#include "world/emperor.hpp"
#include "city_donation_window.hpp"
#include "emperorgiftwindow.hpp"
#include "gui/environment.hpp"
#include "gui/dialogbox.hpp"
#include "texturedbutton.hpp"
#include "dictionary.hpp"
#include "gui/widget_helper.hpp"
#include "game/gift.hpp"

using namespace gfx;
using namespace events;
using namespace  city;

namespace gui
{

namespace advisorwnd
{

namespace {
  Point requestButtonOffset = Point( 0, 55 );
  Size requestButtonSize = Size( 560, 40 );

  enum { favourLimiter=20, maxFavourValue=100 };
}

class RequestButton : public PushButton
{
public:
  RequestButton( Widget* parent, const Point& pos, int index, RequestPtr request )
    : PushButton( parent, Rect( pos + requestButtonOffset * index, requestButtonSize), "", -1, false, PushButton::blackBorderUp )
  {
    _request = request;

    auto goodRequest = _request.as<GoodRequest>();
    if( goodRequest.isValid() )
      _goodPic = good::Helper::picture( goodRequest->goodType() );

    _finalizeResize();

    CONNECT( this, onClicked(), this, RequestButton::_executeRequest );
  }

  virtual void _updateTextPic()
  {
    PushButton::_updateTextPic();

    Picture& pic = _textPicture();

    Font font = Font::create( FONT_1_WHITE );

    auto goodRequest = _request.as<request::RqGood>();
    if( goodRequest.isValid() )
    {
      font.draw( pic, utils::format( 0xff, "%d", goodRequest->qty() ), 2, 2 );
      font.draw( pic, good::Helper::getTypeName( goodRequest->goodType() ), 60, 2 );

      int month2comply = game::Date::current().monthsTo( goodRequest->finishedDate() );
      font.draw( pic, utils::format( 0xff, "%d %s", month2comply, _( "##rqst_month_2_comply##") ), 250, 2 );
      font.draw( pic, goodRequest->description(), 5, pic.height() - 20 );
    }
  }

  virtual void draw(Engine &painter)
  {
    PushButton::draw( painter );
    painter.draw( _goodPic, absoluteRect().lefttop() + Point( 40, 2 ), &absoluteClippingRectRef() );
  }

public signals:
  Signal1<RequestPtr>& onExecRequest() { return _onExecRequestSignal; }

private:
  void _acceptRequest()  { emit _onExecRequestSignal( _request );  }

  void _executeRequest()
  {
    dialog::Dialog* dialog = dialog::Confirmation( ui(),  "", "##dispatch_emperor_request_question##" );
    CONNECT( dialog, onOk(), this, RequestButton::_acceptRequest );
  }

  Signal1<RequestPtr> _onExecRequestSignal;
  RequestPtr _request;
  Picture _goodPic;
};

class Emperor::Impl
{
public:
  PlayerCityPtr city;
  gui::Label* lbEmperorFavour;
  gui::Label* lbEmperorFavourDesc;
  gui::Label* lbPost;
  gui::Label* lbPrimaryFunds;
  PushButton* btnSendGift;
  PushButton* btnSend2City;
  PushButton* btnChangeSalary; 
  GameAutoPause autoPause;
  bool isRequestsUpdated;

  void sendMoney( int money );
  void sendGift( int money );
  void changeSalary(int money );
  void resolveRequest( RequestPtr request );

  std::string getEmperorFavourStr()
  {
    return utils::format( 0xff, "##emperor_favour_%02d##", city->favour() * favourLimiter / maxFavourValue  );
  }
};

void Emperor::_showChangeSalaryWindow()
{
  if( game::Date::current() > _d->city->victoryConditions().finishDate() )
  {
    dialog::Information( ui(), "", _("##disabled_draw_salary_for_free_reign##") );
    return;
  }

  PlayerPtr pl = _d->city->mayor();
  auto dialog = new dialog::ChangeSalary( parent(), pl->salary() );
  dialog->show();

  TexturedButton* btnHelp = new TexturedButton( this, Point( 12, height() - 39), Size( 24 ), -1, ResourceMenu::helpInfBtnPicId );
  CONNECT( btnHelp, onClicked(), this, Emperor::_showHelp );
  CONNECT( dialog, onChangeSalary(), _d.data(), Impl::changeSalary )
}

void Emperor::_showSend2CityWindow()
{
  PlayerPtr pl = _d->city->mayor();
  auto dialog = new dialog::CityDonation( parent(), pl->money() );
  dialog->show();

  CONNECT( dialog, onSendMoney(), _d.data(), Impl::sendMoney );
}

void Emperor::_showGiftWindow()
{
  PlayerPtr pl = _d->city->mayor();
  world::Emperor& emperor = _d->city->empire()->emperor();

  auto dialog = new dialog::EmperorGift( parent(),
                                         pl->money(),
                                         emperor.lastGiftDate( _d->city->name() ) );
  dialog->show();

  CONNECT( dialog, onSendGift(), _d.data(), Impl::sendGift );
}

void Emperor::_updateRequests()
{
  Rect reqsRect( Point( 32, 91 ), Size( 570, 220 ) );

  auto buttons = findChildren<RequestButton*>();
  for( auto btn : buttons )
    btn->deleteLater();

  RequestList requests;
  request::DispatcherPtr dispatcher = _d->city->statistic().services.find<request::Dispatcher>();

  if( dispatcher.isValid() )
  {
    requests = dispatcher->requests();
  }

  if( requests.empty() )
  {
    Label* lb = new Label( this, reqsRect, _("##have_no_requests##") );
    lb->setWordwrap( true );
    lb->setTextAlignment( align::upperLeft, align::center );
  }
  else
  {
    foreach( r, requests )
    {
      if( !(*r)->isDeleted() )
      {
        bool mayExec = (*r)->isReady( _d->city );
        RequestButton* btn = new RequestButton( this, reqsRect.lefttop() + Point( 5, 5 ),
                                                std::distance( requests.begin(), r ), *r );
        btn->setTooltipText( _("##request_btn_tooltip##") );
        btn->setEnabled( mayExec );
        CONNECT(btn, onExecRequest(), _d.data(), Impl::resolveRequest );
      }
    }
  }
  _d->isRequestsUpdated = false;
}

void Emperor::_showHelp()
{
  DictionaryWindow::show( this, "emperor_advisor" );
}

Emperor::Emperor( PlayerCityPtr city, Widget* parent, int id )
: Base( parent, city, id ), _d( new Impl )
{
  _d->autoPause.activate();
  _d->city = city;
  _d->isRequestsUpdated = true;

  Widget::setupUI( ":/gui/emperoropts.gui" );

  INIT_WIDGET_FROM_UI( Label*, lbTitle )

  GET_DWIDGET_FROM_UI( _d, lbEmperorFavour )
  GET_DWIDGET_FROM_UI( _d, lbEmperorFavourDesc  )
  GET_DWIDGET_FROM_UI( _d, lbPost )
  GET_DWIDGET_FROM_UI( _d, lbPrimaryFunds )
  GET_DWIDGET_FROM_UI( _d, btnSendGift )
  GET_DWIDGET_FROM_UI( _d, btnSend2City )
  GET_DWIDGET_FROM_UI( _d, btnChangeSalary )

  if( _d->lbEmperorFavour )
    _d->lbEmperorFavour->setText( utils::format( 0xff, "%s %d", _("##advemp_emperor_favour##"), _d->city->favour() ) );

  if( _d->lbEmperorFavourDesc )
    _d->lbEmperorFavourDesc->setText( _( _d->getEmperorFavourStr() ) );

  if( lbTitle )
  {
    std::string text = city->mayor()->name();
    if( text.empty() )
      text = _("##emperor_advisor_title##");

    lbTitle->setText( text );
  }

  CONNECT( _d->btnChangeSalary, onClicked(), this, Emperor::_showChangeSalaryWindow );
  CONNECT( _d->btnSend2City, onClicked(), this, Emperor::_showSend2CityWindow );
  CONNECT( _d->btnSendGift, onClicked(), this, Emperor::_showGiftWindow );
}

void Emperor::draw(gfx::Engine& painter )
{
  if( !visible() )
    return;

  if( _d->isRequestsUpdated )
  {
    _updateRequests();
  }

  Window::draw( painter );
}

void Emperor::Impl::sendMoney( int money )
{
  city->mayor()->appendMoney( -money );
  GameEventPtr e = Payment::create( econ::Issue::donation, money );
  e->dispatch();
}

void Emperor::Impl::sendGift(int money)
{
  if( money > city->mayor()->money() )
  {
    dialog::Information( lbEmperorFavour->ui(),
                         _("##nomoney_for_gift_title##"),
                         _("##nomoney_for_gift_text##") );
    return;
  }

  city->mayor()->appendMoney( -money );
  city->empire()->emperor().sendGift( Gift( city->name(), "gift", money ) );
}

void Emperor::Impl::changeSalary( int money )
{
  PlayerPtr pl = city->mayor();
  pl->setSalary( money );

  float salKoeff = world::EmpireHelper::governorSalaryKoeff( ptr_cast<world::City>( city ) );
  if( salKoeff > 1.f )
  {
    dialog::Information( lbEmperorFavour->ui(),
                         _("##changesalary_warning##"),
                         _("##changesalary_greater_salary##") );
  }
}

void Emperor::Impl::resolveRequest(RequestPtr request)
{
  if( request.isValid() )
  {
    request->exec( city );
    isRequestsUpdated = true;
  }
}

}//end namespace advisorwnd

}//end namespace gui
