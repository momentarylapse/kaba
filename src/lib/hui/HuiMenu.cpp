/*----------------------------------------------------------------------------*\
| Hui menu                                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2010.01.31 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "hui.h"
#include "Controls/HuiMenuItem.h"
#include "Controls/HuiMenuItemToggle.h"
#include "Controls/HuiMenuItemSubmenu.h"
#include "Controls/HuiMenuItemSeparator.h"


void HuiMenu::Clear()
{
	foreach(HuiControl *c, item)
		delete(c);
	item.clear();
}

void HuiMenu::AddItem(const string &name, const string &id)
{
	add(new HuiMenuItem(name, id));
}

void HuiMenu::AddItemImage(const string &name, const string &image, const string &id)
{
	add(new HuiMenuItem(name, id));
	item.back()->SetImage(image);
}

void HuiMenu::AddItemCheckable(const string &name, const string &id)
{
	add(new HuiMenuItemToggle(name, id));
}

void HuiMenu::AddSeparator()
{
	add(new HuiMenuItemSeparator());
}

void HuiMenu::AddSubMenu(const string &name, const string &id, HuiMenu *menu)
{
	if (menu)
		add(new HuiMenuItemSubmenu(name, menu, id));
}

void HuiMenu::set_win(HuiWindow *_win)
{
	win = _win;
	foreach(HuiControl *c, item){
		c->win = win;
		HuiMenuItemSubmenu *s = dynamic_cast<HuiMenuItemSubmenu*>(c);
		if (s)
			s->sub_menu->set_win(win);
	}
}

// only allow menu callback, if we are in layer 0 (if we don't edit it ourself)
int allow_signal_level = 0;

// stupid function for HuiBui....
void HuiMenu::SetID(const string &id)
{
}

HuiMenu *HuiMenu::GetSubMenuByID(const string &id)
{
	foreach(HuiControl *c, item){
		HuiMenuItemSubmenu *s = dynamic_cast<HuiMenuItemSubmenu*>(c);
		if (s){
			if (s->id == id)
				return s->sub_menu;
			HuiMenu *m = s->sub_menu->GetSubMenuByID(id);
			if (m)
				return m;
		}
	}
	return NULL;
}


void HuiMenu::UpdateLanguage()
{
	msg_db_f("UpdateMenuLanguage", 1);
#if 0
	foreach(HuiMenuItem &it, item){
		if (it.sub_menu)
			it.sub_menu->UpdateLanguage();
		if ((it.id.num == 0) || (it.is_separator))
			continue;
		bool enabled = it.enabled;
		/*  TODO
		#ifdef HUI_API_WIN
			if (strlen(get_lang(it.ID, "", true)) > 0){
				strcpy(it.Name, HuiGetLanguage(it.ID));
				ModifyMenu(m->hMenu, i, MF_STRING | MF_BYPOSITION, it.ID, get_lang_sys(it.ID, "", true));
			}
		#endif
		*/
		string s = get_lang(it.id, "", true);
		if (s.num > 0)
			SetText(it.id, s);
		msg_todo("HuiUpdateMenuLanguage (GTK) (menu bar)");
			//gtk_menu_item_set_label(GTK_MENU_ITEM(it.g_item), get_lang_sys(it.ID, "", true));
		EnableItem(it.id, enabled);
	}
#endif
}

Array<HuiControl*> HuiMenu::get_all_controls()
{
	Array<HuiControl*> list = item;
	foreach(HuiControl *c, item){
		HuiMenuItemSubmenu *s = dynamic_cast<HuiMenuItemSubmenu*>(c);
		if (s)
			list.append(s->sub_menu->get_all_controls());
	}
	return list;
}

HuiMenu *HuiCreateMenu()
{
	return new HuiMenu();
}
