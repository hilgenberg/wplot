#pragma once
#include "cnum.h"

struct Point
{
	static const int OVERLAP = 1; // how many neighbouring points does a point's calculation need?

	cnum e, de;
	//cnum g, dg;

	inline void clear()
	{
		memset(this, 0, sizeof(Point));
	}

	void init(double x, double y); // x in [-1,1], y in h/w*[-1,1]
	void evolve(const Point *p0, const int dy); // p0 is the point from last iteration, p0[dy] is the point above
	void display(unsigned char pixel[4]); // Point --> RGBA
};
