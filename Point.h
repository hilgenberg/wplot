#pragma once
#include "cnum.h"
#include "Graphs/Vector.h"

//#define ZERO_BORDER

#define DIRAC 0
#define MAXWELL 1
#define KLEINGORDON 2

#define EQUATION DIRAC
//#define EQUATION MAXWELL
//#define EQUATION KLEINGORDON

#if EQUATION==DIRAC
#define POINT_SIZE 4
#define e0 F[0]
#define e1 F[1]
#define e2 F[2]
#define e3 F[3]
#else
#define POINT_SIZE 2
#define e  F[0]
#define de F[1]
#endif

struct Point
{
	static const int OVERLAP = 1; // how far into neighbouring points does a point's calculation read?
	static const int MOD_OVERLAP = 0; // how far does it modify?
	static int vis;
	static int Y; // P[-1] is the left neighbour, P[1] the right, P[-Y] above and P[Y] below.
	
	cnum F[POINT_SIZE]; // whatever fields the model uses
	P3d  g; // (x,y,z) = (g_x, g_y, sqrt(1-g_t))

	inline void clear()
	{
		g.clear();
		memset(F, 0, POINT_SIZE*sizeof(cnum));
	}

	void init(double x, double y, double Y); // x in [-1,1], y in [-Y,Y], Y = h/w
	void evolve(const Point *p0); // p0 is the point from last iteration
	void display(unsigned char pixel[4]); // Point --> RGBA
	void operator+= (const Point &p)
	{
		assert(MOD_OVERLAP > 0); // otherwise there should be no need to call this
		for (int i = 0; i < POINT_SIZE; ++i) F[i] += p.F[i];
	}

private:
	void init_g(double x, double y);

	// modified differential operators for QG (constant factors like 1/dx^2 ignored):
	inline cnum laplace(int i = 0) const
	{
		return
		this[-1].F[i] * (1.0-g.x) +
		this[+1].F[i] * (1.0+g.x) +
		this[-Y].F[i] * (1.0-g.y) +
		this[+Y].F[i] * (1.0+g.y) - F[i] * 4.0;
	}
	inline cnum laplace_orig(int i = 0) const
	{
		return
		this[-1].F[i] +
		this[+1].F[i] +
		this[-Y].F[i] +
		this[+Y].F[i] - F[i] * 4.0;
	}
	// first order (df/dx, df/dy):
	inline cnum dfdx(int i = 0) const
	{
		return 
		(this[+1].F[i] * (1.0+g.x) - this[-1].F[i] * (1.0-g.x)) * 0.5 - F[i] * g.x;
	}
	inline cnum dfdy(int i = 0) const
	{
		return 
		(this[+Y].F[i] * (1.0+g.y) - this[-Y].F[i] * (1.0-g.y)) * 0.5 - F[i] * g.y;
	}
	// impulse (for display):
	inline double px(int i = 0)
	{
		return sp(ix(this[-1].F[i] - this[+1].F[i]), F[i]) * 0.5;
	}
	inline double py(int i = 0)
	{
		return sp(ix(this[-Y].F[i] - this[+Y].F[i]), F[i]) * 0.5;
	}
};
