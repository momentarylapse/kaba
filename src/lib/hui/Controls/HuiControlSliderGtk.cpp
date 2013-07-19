/*
 * HuiControlSlider.cpp
 *
 *  Created on: 17.06.2013
 *      Author: michi
 */

#include "HuiControlSlider.h"

#ifdef HUI_API_GTK

void OnGtkSliderChange(GtkWidget *widget, gpointer data)
{	((HuiControl*)data)->Notify("hui:change");	}

HuiControlSlider::HuiControlSlider(const string &title, const string &id, bool horizontal) :
	HuiControl(HuiKindSlider, id)
{
	GetPartStrings(id, title);
	if (horizontal){
#if GTK_MAJOR_VERSION >= 3
		widget = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0.0, 1.0, 0.0001);
#else
		widget = gtk_vscale_new_with_range(0.0, 1.0, 0.0001);
#endif
		gtk_range_set_inverted(GTK_RANGE(widget), true);
	}else{
#if GTK_MAJOR_VERSION >= 3
		widget = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.0001);
#else
		widget = gtk_vscale_new_with_range(0.0, 1.0, 0.0001);
#endif
	}
	gtk_scale_set_draw_value(GTK_SCALE(widget), false);
	g_signal_connect(G_OBJECT(widget), "value-changed", G_CALLBACK(&OnGtkSliderChange), this);
}

HuiControlSlider::~HuiControlSlider() {
	// TODO Auto-generated destructor stub
}

float HuiControlSlider::GetFloat()
{
	return (float)gtk_range_get_value(GTK_RANGE(widget));
}

void HuiControlSlider::__SetFloat(float f)
{
	gtk_range_set_value(GTK_RANGE(widget), f);
}

#endif
