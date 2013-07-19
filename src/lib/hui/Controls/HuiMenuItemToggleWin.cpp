/*
 * HuiMenuItemToggleWin.cpp
 *
 *  Created on: 13.07.2013
 *      Author: michi
 */

#include "HuiMenuItemToggle.h"

#ifdef HUI_API_WIN

HuiMenuItemToggle::HuiMenuItemToggle(const string &title, const string &id) :
	HuiControl(HuiKindMenuItemToggle, id)
{
}

HuiMenuItemToggle::~HuiMenuItemToggle()
{
}

void HuiMenuItemToggle::__Check(bool checked)
{
}

bool HuiMenuItemToggle::IsChecked()
{
	return false;
}

#endif
