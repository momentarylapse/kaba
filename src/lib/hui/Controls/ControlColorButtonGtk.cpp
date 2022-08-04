/*
 * ControlColorButton.cpp
 *
 *  Created on: 17.06.2013
 *      Author: michi
 */

#include "ControlColorButton.h"
#include "../Event.h"
#include "../../image/color.h"

#ifdef HUI_API_GTK

namespace hui
{


void OnGtkColorButtonChange(GtkWidget *widget, gpointer data)
{	reinterpret_cast<Control*>(data)->notify(EventID::CHANGE);	}

ControlColorButton::ControlColorButton(const string &title, const string &id) :
	Control(CONTROL_COLORBUTTON, id)
{
	auto parts = split_title(title);
	widget = gtk_color_button_new();
	take_gtk_ownership();
	//g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(&OnGtkButtonPress), this);
	g_signal_connect(G_OBJECT(widget), "color-set", G_CALLBACK(&OnGtkColorButtonChange), this);
}

int col_f_to_i16(float f) {
	return (int)(f * 65535.0f);
}

float col_i16_to_f(int i) {
	return (float)i / 65535.0f;
}


GdkRGBA color_to_gdk(const color &c) {
	GdkRGBA gcol;
	gcol.red = c.r;
	gcol.green = c.g;
	gcol.blue = c.b;
	gcol.alpha = c.a;
	return gcol;
}

color color_from_gdk(const GdkRGBA &gcol) {
	color col;
	col.r = (float)gcol.red;
	col.g = (float)gcol.green;
	col.b = (float)gcol.blue;
	col.a = (float)gcol.alpha;
	return col;
}

void ControlColorButton::__set_color(const color& c) {
	GdkRGBA gcol = color_to_gdk(c);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(widget), &gcol);
}

color ControlColorButton::get_color() {
	GdkRGBA gcol;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widget), &gcol);
	return color_from_gdk(gcol);
}

void ControlColorButton::__set_option(const string &op, const string &value) {
	if (op == "alpha")
		gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(widget), true);
}

};

#endif
