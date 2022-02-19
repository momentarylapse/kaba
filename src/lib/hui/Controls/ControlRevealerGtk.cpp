/*
 * ControlRevealerGtk.cpp
 *
 *  Created on: 17.05.2015
 *      Author: michi
 */

#include "ControlRevealer.h"

#ifdef HUI_API_GTK

namespace hui {

void control_link(Control *parent, Control *child);
void control_unlink(Control *parent, Control *child);

void on_gtk_expander_expand(GObject* object, GParamSpec *param_spec, gpointer user_data);

ControlRevealer::ControlRevealer(const string &title, const string &id) :
	Control(CONTROL_REVEALER, id)
{
	auto parts = split_title(title);

#if GTK_CHECK_VERSION(3,10,0)
	widget = gtk_revealer_new();
#else
	widget = gtk_expander_new(sys_str("<b>Revealer...</b>"));
	gtk_expander_set_use_markup(GTK_EXPANDER(widget), true);
	g_signal_connect(widget, "notify::expanded", G_CALLBACK(on_gtk_expander_expand), nullptr);
	if (!gtk_expander_get_expanded(GTK_EXPANDER(widget)))
		gtk_widget_set_vexpand(widget, false);
#endif

	set_options(get_option_from_title(title));
}


void ControlRevealer::add(Control *child, int x, int y) {
	GtkWidget *child_widget = child->get_frame();
#if GTK_CHECK_VERSION(4,0,0)
	gtk_revealer_set_child(GTK_REVEALER(widget), child_widget);
#else
	gtk_container_add(GTK_CONTAINER(widget), child_widget);
#endif
	control_link(this, child);
}

void ControlRevealer::remove_child(Control *child) {
	msg_write("Revealer.remove");
#if GTK_CHECK_VERSION(4,0,0)
	gtk_revealer_set_child(GTK_REVEALER(widget), nullptr);
#else
	GtkWidget *child_widget = child->get_frame();
	gtk_container_remove(GTK_CONTAINER(widget), child_widget);
#endif
	control_unlink(this, child);
}

void ControlRevealer::reveal(bool reveal) {
#if GTK_CHECK_VERSION(3,10,0)
	gtk_revealer_set_reveal_child(GTK_REVEALER(widget), reveal);
#else
	gtk_expander_set_expanded(GTK_EXPANDER(widget), reveal);
#endif
}

bool ControlRevealer::is_revealed() {
#if GTK_CHECK_VERSION(3,10,0)
	return gtk_revealer_get_reveal_child(GTK_REVEALER(widget));
#else
	return (bool)gtk_expander_get_expanded(GTK_EXPANDER(widget));
#endif
}


void ControlRevealer::__set_option(const string& op, const string& value) {
#if GTK_CHECK_VERSION(3,10,0)
	if (op == "slide"){
		if (value == "up")
			gtk_revealer_set_transition_type(GTK_REVEALER(widget), GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
		else if (value == "down")
			gtk_revealer_set_transition_type(GTK_REVEALER(widget), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
		else if (value == "left")
			gtk_revealer_set_transition_type(GTK_REVEALER(widget), GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
		else if (value == "right")
			gtk_revealer_set_transition_type(GTK_REVEALER(widget), GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
	}else if (op == "crossfade")
		gtk_revealer_set_transition_type(GTK_REVEALER(widget), GTK_REVEALER_TRANSITION_TYPE_CROSSFADE);
#endif
}

};

#endif
