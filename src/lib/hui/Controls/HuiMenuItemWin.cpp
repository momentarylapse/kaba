/*
 * HuiMenuItemWin.cpp
 *
 *  Created on: 13.07.2013
 *      Author: michi
 */

#include "HuiMenuItem.h"
#include "../hui.h"
#include "../hui_internal.h"

#ifdef HUI_API_WIN

HuiMenuItem::HuiMenuItem(const string &title, const string &id) :
	HuiControl(HuiKindMenuItem, id)
{
}

HuiMenuItem::~HuiMenuItem()
{
}

void HuiMenuItem::SetImage(const string &image)
{
}

#endif
