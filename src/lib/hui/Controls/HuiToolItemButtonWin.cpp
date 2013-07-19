/*
 * HuiToolItemButtonWin.cpp
 *
 *  Created on: 13.07.2013
 *      Author: michi
 */

#include "HuiToolItemButton.h"


#ifdef HUI_API_WIN

HuiToolItemButton::HuiToolItemButton(const string &title, const string &image, const string &id) :
	HuiControl(HuiKindToolButton, id)
{
}

HuiToolItemButton::~HuiToolItemButton()
{
}

#endif
