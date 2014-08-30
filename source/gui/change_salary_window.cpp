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

#include "change_salary_window.hpp"
#include "pushbutton.hpp"
#include "listbox.hpp"
#include "core/logger.hpp"
#include "world/empire.hpp"
#include "core/gettext.hpp"
#include "widget_helper.hpp"
#include "core/stringhelper.hpp"

namespace gui
{

class ChangeSalaryWindow::Impl
{
public:
  int newSalary;

public oc3_slots:
  void resolveSalaryChange(const ListBoxItem&);
  void setNewSalary();

public oc3_signals:
  Signal1<int> onChangeSalarySignal;
};

ChangeSalaryWindow::ChangeSalaryWindow(Widget* p, unsigned int salary)
  : Window( p, Rect(), "" ), __INIT_IMPL(ChangeSalaryWindow)
{
  setupUI( ":/gui/changesalary.gui");
  setCenter( parent()->center() );

  _dfunc()->newSalary = salary;

  ListBox* lbxTitles;
  GET_WIDGET_FROM_UI( lbxTitles )
  if( lbxTitles )
  {
    world::GovernorRanks ranks = world::EmpireHelper::ranks();
    foreach( i, ranks )
    {
      std::string salaryStr = _( "##" + (*i).rankName + "_salary##" );
      ListBoxItem& item = lbxTitles->addItem( salaryStr + "   " + StringHelper::i2str( (*i).salary ) );
      item.setTag( (*i).salary );
      if( (*i).salary == salary )
      {
        lbxTitles->setSelected( std::distance( ranks.begin(), i ) );
      }
    }
  }

  PushButton* btnCancel;
  PushButton* btnOk;
  GET_WIDGET_FROM_UI( btnCancel  )
  GET_WIDGET_FROM_UI( btnOk )

  CONNECT( btnCancel, onClicked(), this, ChangeSalaryWindow::deleteLater );
  CONNECT( btnOk, onClicked(), _dfunc().data(), Impl::setNewSalary );
  CONNECT( btnOk, onClicked(), this, ChangeSalaryWindow::deleteLater  );
  CONNECT( lbxTitles, onItemSelected(), _dfunc().data(), Impl::resolveSalaryChange );
}

ChangeSalaryWindow::~ChangeSalaryWindow(){}
Signal1<int>& ChangeSalaryWindow::onChangeSalary(){  return _dfunc()->onChangeSalarySignal; }
void ChangeSalaryWindow::Impl::resolveSalaryChange(const ListBoxItem& item ) { newSalary = item.tag(); }
void ChangeSalaryWindow::Impl::setNewSalary(){ oc3_emit onChangeSalarySignal( newSalary ); }


}//end namespace gui
