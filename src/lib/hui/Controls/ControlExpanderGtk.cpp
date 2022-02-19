/*
 * ControlExpanderGtk.cpp
 *
 *  Created on: 18.09.2013
 *      Author: michi
 */

#include "ControlExpander.h"
#include "../Window.h"

#ifdef HUI_API_GTK

namespace hui {

void control_link(Control *parent, Control *child);
void control_unlink(Control *parent, Control *child);

const int FRAME_INDENT = 0; //20;

void on_gtk_expander_expand(GObject* object, GParamSpec *param_spec, gpointer user_data) {
	if (gtk_expander_get_expanded(GTK_EXPANDER(object))) {
		gtk_widget_set_vexpand_set(GTK_WIDGET(object), false);
	} else {
		gtk_widget_set_vexpand(GTK_WIDGET(object), false);
	}
}

ControlExpander::ControlExpander(const string &title, const string &id) :
	Control(CONTROL_EXPANDER, id)
{
	auto parts = split_title(title);
	widget = gtk_expander_new(sys_str("<b>" + parts[0] + "</b>"));
	gtk_expander_set_use_markup(GTK_EXPANDER(widget), true);

	g_signal_connect(widget, "notify::expanded", G_CALLBACK(on_gtk_expander_expand), nullptr);
	if (!gtk_expander_get_expanded(GTK_EXPANDER(widget)))
		gtk_widget_set_vexpand(widget, false);
}

void ControlExpander::expand(int row, bool expand) {
	gtk_expander_set_expanded(GTK_EXPANDER(widget), expand);
}

void ControlExpander::expand_all(bool expand) {
	gtk_expander_set_expanded(GTK_EXPANDER(widget), expand);
}

bool ControlExpander::is_expanded(int row) {
	return (bool)gtk_expander_get_expanded(GTK_EXPANDER(widget));
}

void ControlExpander::add(Control *child, int x, int y) {
	GtkWidget *child_widget = child->get_frame();
	//gtk_widget_set_vexpand(child_widget, true);
	//gtk_widget_set_hexpand(child_widget, true);
	int ind = child->indent;
	if (ind < 0)
		ind = FRAME_INDENT;
#if GTK_CHECK_VERSION(3,12,0)
	gtk_widget_set_margin_start(child_widget, ind);
#else
	gtk_widget_set_margin_left(child_widget, ind);
#endif
#if GTK_CHECK_VERSION(4,0,0)
	gtk_expander_set_child(GTK_EXPANDER(widget), child_widget);
#else
	gtk_container_add(GTK_CONTAINER(widget), child_widget);
#endif
	control_link(this, child);
}

void ControlExpander::remove_child(Control *child) {
	GtkWidget *child_widget = child->get_frame();
#if GTK_CHECK_VERSION(4,0,0)
	gtk_expander_set_child(GTK_EXPANDER(widget), nullptr);
#else
	gtk_container_remove(GTK_CONTAINER(widget), child_widget);
#endif
	control_unlink(this, child);
}

};

#endif
