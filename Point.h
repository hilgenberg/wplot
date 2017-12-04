#pragma once
#include "cnum.h"
#include "Graphs/Vector.h"

//#define DIRAC
//#define MAXWELL

struct Point
{
	static const int OVERLAP = 1; // how many neighbouring points does a point's calculation need?

	#ifdef DIRAC
	cnum e0, e1, e2, e3;
	#else
	cnum e, de;
	#endif

	P3d g;

	inline void clear()
	{
		memset(this, 0, sizeof(Point));
	}

	void init(double x, double y, double Y); // x in [-1,1], y in [-Y,Y], Y = h/w
	void evolve(const Point *p0, const int dy); // p0 is the point from last iteration, p0[dy] is the point above
	void display(unsigned char pixel[4]); // Point --> RGBA

private:
	void init_g(double x, double y);
};
