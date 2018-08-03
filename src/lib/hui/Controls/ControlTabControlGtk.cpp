/*
 * ControlTabControl.cpp
 *
 *  Created on: 17.06.2013
 *      Author: michi
 */

#include "../hui.h"
#include "ControlTabControl.h"

#ifdef HUI_API_GTK

namespace hui
{

void OnGtkTabControlSwitch(GtkWidget *widget, GtkWidget *page, guint page_num, gpointer data)
{
	ControlTabControl *c = reinterpret_cast<ControlTabControl*>(data);
	c->cur_page = page_num;
	c->notify("hui:change");
}


ControlTabControl::ControlTabControl(const string &title, const string &id, Panel *panel) :
	Control(CONTROL_TABCONTROL, id)
{
	GetPartStrings(title);
	widget = gtk_notebook_new();
	this->panel = panel; // for addPage()

	if (PartString.num > 1 or PartString[0] != "")
		for (int i=0;i<PartString.num;i++)
			addPage(PartString[i]);

	cur_page = 0;
	g_signal_connect(G_OBJECT(widget), "switch-page", G_CALLBACK(&OnGtkTabControlSwitch), this);
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(widget), true);
	setOptions(OptionString);
}

ControlTabControl::~ControlTabControl()
{
	gtk_widget_hide(widget);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(widget), false);
}

string ControlTabControl::getString()
{
	return "";
}

void ControlTabControl::__setString(const string& str)
{
}

void ControlTabControl::__setInt(int i)
{
	gtk_notebook_set_current_page(GTK_NOTEBOOK(widget), i);
	cur_page = i;
}

int ControlTabControl::getInt()
{
	return cur_page;
}

void ControlTabControl::__addString(const string &str)
{
	addPage(str);
}

void ControlTabControl::__removeString(int row)
{
	if (row >= 0 and row < pages.num){
		delete pages[row];
		pages.erase(row);
	}
	gtk_notebook_remove_page(GTK_NOTEBOOK(widget), row);
}

void ControlTabControl::__changeString(int row, const string& str)
{
	auto *child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widget), row);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(widget), child, str.c_str());
}

void ControlTabControl::addPage(const string &str)
{
	GtkWidget *inside;
#if GTK_CHECK_VERSION(3,0,0)
	inside = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	inside = gtk_hbox_new(true, 0);
#endif
	gtk_box_set_homogeneous(GTK_BOX(inside), true);
	gtk_widget_show(inside);
	GtkWidget *label = gtk_label_new(sys_str(str));
	gtk_notebook_append_page(GTK_NOTEBOOK(widget), inside, label);
}

void ControlTabControl::add(Control *child, int x, int y)
{
	if (x >= pages.num)
		pages.resize(x + 1);
	if (pages[x])
		msg_error("overwriting existing tab page... in " + id);
	pages[x] = child;

	GtkWidget *child_widget = child->get_frame();
	GtkWidget *target_widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(widget), x);
	gtk_container_add(GTK_CONTAINER(target_widget), child_widget);
	if (panel)
		gtk_container_set_border_width(GTK_CONTAINER(target_widget), panel->border_width);
	children.add(child);
	child->parent = this;
}

void ControlTabControl::__setOption(const string &op, const string &value)
{
	if (op == "nobar")
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(widget), false);
	else if (op == "bar")
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(widget), value._bool());
	else if (op == "top")
		gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget), GTK_POS_TOP);
	else if (op == "bottom")
		gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget), GTK_POS_BOTTOM);
	else if (op == "left"){
		gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget), GTK_POS_LEFT);
		gtk_notebook_set_scrollable(GTK_NOTEBOOK(widget), false);
	}else if (op == "right"){
		gtk_notebook_set_tab_pos(GTK_NOTEBOOK(widget), GTK_POS_RIGHT);
		gtk_notebook_set_scrollable(GTK_NOTEBOOK(widget), false);
	}
}

};

#endif
