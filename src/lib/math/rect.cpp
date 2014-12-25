#include "math.h"

rect::rect(float x1, float x2, float y1, float y2)
{
	this->x1 = x1;
	this->x2 = x2;
	this->y1 = y1;
	this->y2 = y2;
}

string rect::str() const
{
	return format("(%f, %f, %f, %f)", x1, x2, y1, y2);
}

float rect::width() const
{
	return x2 - x1;
}

float rect::height() const
{
	return y2 - y1;
}

float rect::area() const
{
	return (x2 - x1) * (y2 - y1);
}

bool rect::inside(float x, float y) const
{
	return (x >= x1) and (x <= x2) and (y >= y1) and (y <= y2);
}

bool rect::operator ==(const rect &r) const
{
	return (x1 == r.x1) and (y1 == r.y1) and (x2 == r.x2) and (y2 == r.y2);
}

bool rect::operator !=(const rect &r) const
{
	return !(*this == r);
}
