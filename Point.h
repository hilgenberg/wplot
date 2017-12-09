#pragma once
#include "cnum.h"
#include "Graphs/Vector.h"

#define ZERO_BORDER
#define DIRAC
//#define MAXWELL

struct Point
{
	static const int OVERLAP = 1; // how far into neighbouring points does a point's calculation read?
	static const int MOD_OVERLAP = 0; // how far does it modify?

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
	void operator+= (const Point &p)
	{
		assert(MOD_OVERLAP > 0); // otherwise there should be no need to call this
		#ifdef DIRAC
		e0 += p.e0;
		e1 += p.e1;
		e2 += p.e2;
		e3 += p.e3;
		#else
		e += p.e;
		de += p.de;
		#endif
	}

private:
	void init_g(double x, double y);
};
