/*
 * HuiMenuItemSubmenuWin.cpp
 *
 *  Created on: 13.07.2013
 *      Author: michi
 */

#include "HuiMenuItemSubmenu.h"
#include "../HuiMenu.h"

#ifdef HUI_API_WIN

HuiMenuItemSubmenu::HuiMenuItemSubmenu(const string &title, HuiMenu *menu, const string &id) :
	HuiControl(HuiKindMenuItemSubmenu, id)
{
}

HuiMenuItemSubmenu::~HuiMenuItemSubmenu()
{
}

#endif

