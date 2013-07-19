/*
 * HuiToolItemMenuButtonWin.cpp
 *
 *  Created on: 13.07.2013
 *      Author: michi
 */

#include "HuiToolItemMenuButton.h"
#include "../HuiMenu.h"

#ifdef HUI_API_WIN

HuiToolItemMenuButton::HuiToolItemMenuButton(const string &title, HuiMenu *menu, const string &image, const string &id) :
	HuiControl(HuiKindToolMenuButton, id)
{
}

HuiToolItemMenuButton::~HuiToolItemMenuButton()
{
}

#endif

