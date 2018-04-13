/*
 * ControlDrawingArea.h
 *
 *  Created on: 17.06.2013
 *      Author: michi
 */

#ifndef CONTROLDRAWINGAREA_H_
#define CONTROLDRAWINGAREA_H_

#include "Control.h"

class rect;

namespace hui
{

class Timer;

class ControlDrawingArea : public Control
{
public:
	ControlDrawingArea(const string &text, const string &id);
	~ControlDrawingArea();
	void make_current();

	void *cur_cairo;
	bool is_opengl;

	Array<rect> redraw_area;
	Timer *delay_timer;

	void redraw();
	void redraw(const rect &r);
};

};

#endif /* CONTROLDRAWINGAREA_H_ */
