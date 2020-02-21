#include "Point.h"
#include "Graphs/GL_Util.h"
#include "random.h"

static inline double sqr(double x) { return x*x; }

int Point::Y = 0;

void Point::init(double x, double y, double Y)
{
	#if EQUATION==DIRAC

	P2d v(38.0, 20.0);
	x -= 0.3;
	double r = 2.0*(x*x + y*y);
	if (r < 0.25)
	{
		clear();
		double s = sin(v.x*x + v.y*y), c = cos(v.x*x + v.y*y);
		double f = 6.0 * exp(-1.0 / (1.0 - 4.0*r));
		e0 = f*cnum(c, s);
		e1 = 0.0;
		e2 = v.y*f*cnum(-s,c);
		e3 = v.x*f*cnum(-s, -c);
	}
	else
	{
		clear();
	}
	x += 0.3;

	#elif EQUATION==MAXWELL
	
	const double r = 0.13, x0 = .4;
	if (abs(x-x0) < r && abs(y) < r)
	{
		double s = sin(123 * x), c = cos(123 * x);
		double f = 1.5* M_E * exp(-1.0 / (1.0 - sqr((x-x0)/r)));
		e = cnum(c, s) *f;
		de = cnum(-s, c)*f;// cnum(c, s)*f;
	}
	else
	{
		clear();
	}
	
	#elif EQUATION==KLEINGORDON
	
	P2d v(38.0, 0.0);
	double r = 2.0*(x*x + y*y);
	if (r < 0.25)
	{
		double s = sin(v.x*x + v.y*y), c = cos(v.x*x + v.y*y);
		double f = 6.0 * exp(-1.0 / (1.0 - 4.0*r));
		e = cnum(s, c) * f;
		de = cnum(c, -s) * f;
	}
	else
	{
		clear();
	}
	
	#endif

	init_g(x, y);
}

void Point::init_g(double x, double y)
{
	g.clear();
	double r = std::hypot(x, y); // (0,0) is at center of screen
	switch (2)
	{
		case 0: // nothing
			break;
		case 1: // mass at the bottom
			g.z = 1.0 / (5.01 - y);
			break;
		case 2: // mass at the left
			g.z = 1.0 / (2.01 - x); // 1/3 --> 1
			break;
		case 3: // mass at the right
			g.z = 1.0 / (2.01 + x); // 1 --> 1/3
			break;

		case 4: // black hole
		{
			const double rh = .15;
			r = std::max(1e-8, r);
			g.z = r < rh ? 1.0 : sqr(rh / r);
			break;
		}
		case 5: // rotating black hole
		{
			const double rh = .15, f = (r < rh ? 1.0 : rh / r);
			r = std::max(1e-8, r);
			g.set(0.5*f*y / r, -0.5*f*x / r, f*f);
			break;
		}
		case 6: // movement only
		{
			g.set(0.5, 0., 0.0);
			break;
		}
		case 7: // only rotating
		{
			r = std::max(1e-8, r);
			g.set(0.1*y / r, -0.1*x / r, 0.0);
			break;
		}
		case 8: // dark energy at the bottom
			g.z = -2.0 / sqr(2.0 - y); // => sqrt(1-g00) > 1
			break;
		case 9: // chaos
			g.set(0.1*normal_rand(), 0.1*normal_rand(), 0.1*normal_rand());
			break;
		case 10: // constant (gxx,gyy)
			g.y = 0.4;
			break;
	}

	// move the sqrt out of evolve() for now (though g.z > 1 could be
	// interesting later (fix precision problems first)!)
	g.z = sqrt(1.0 - g.z);
}

void Point::evolve(const Point *P)
{
	g = P->g; // static gravity for now

	#if EQUATION==DIRAC //---------------------------------------
	const double dt = 0.1 * g.z;
	//constexpr double dx = 1.0;
	static constexpr cnum I(0.0, 1.0);
	#if 1
	static constexpr cnum c03 = -I, c02 = -1.0;
	static constexpr cnum c12 =  0, c13 =  0.0;
	static constexpr cnum c21 =  0, c20 = -1.0;
	static constexpr cnum c30 =  I, c31 =  0.0;
	static constexpr double m0 = 1.0, m1 = 0, m2 = 0, m3 = 0;
	#else
	static constexpr cnum c03 = -I, c02 = -1.0;
	static constexpr cnum c12 =  I, c13 =  0.0;
	static constexpr cnum c21 = -I, c20 = -1.0;
	static constexpr cnum c30 =  I, c31 =  1.0;
	static constexpr double m0 = 1.0, m1 = 1.0, m2 = 1.0, m3 = 1.0;
	#endif
	e0 += P->e0 - (c03*P->dfdx(3) - c02*P->dfdy(2) - m0*ix(P->e0)) * dt;
	e1 += P->e1 - (c12*P->dfdx(2) - c13*P->dfdy(3) - m1*ix(P->e1)) * dt;
	e2 += P->e2 - (c21*P->dfdx(1) - c20*P->dfdy(0) - m2*ix(P->e2)) * dt;
	e3 += P->e3 - (c30*P->dfdx(0) - c31*P->dfdy(1) - m3*ix(P->e3)) * dt;

	#elif EQUATION==MAXWELL

	double dt = 0.1 * g.z;
	constexpr double dx = 1.0;

	de = P->de + P->laplace() * (dt / (dx*dx));
	e = P->e + de * dt;
	
	#elif EQUATION==KLEINGORDON
	
	double dt = 0.1 * g.z;
	constexpr double dx = 1.0;

	#if 1
	de = P->de + (P->laplace()/(dx*dx) - P->e) * dt;
	e = P->e + de * dt;
	#elif 0
	double v = sqr(g.x)+sqr(g.y);
	//double f2 = 1.0-sqr(g.x)-sqr(g.y);
	cnum dde = P->laplace_orig()/(dx*dx) - P->e;
	de = P->de * (1.0-v) + dde * dt * (1.0-v) + 
		(P[-1].de * (1.0-g.x) + P[+1].de * (1.0+g.x))*sqr(g.x)*0.5 +
		(P[-Y].de * (1.0-g.y) + P[+Y].de * (1.0+g.y))*sqr(g.y)*0.5;
	e = P->e * (1.0-v) + de * dt * (1.0-v) + 
		(P[-1].e * (1.0-g.x) + P[+1].e * (1.0+g.x))*sqr(g.x)*0.5 +
		(P[-Y].e * (1.0-g.y) + P[+Y].e * (1.0+g.y))*sqr(g.y)*0.5;
	#else
	e = (P[g.x < 0 ? -1 : 1].e * fabs(g.x) + (1.0-fabs(g.x)) * P->e);

	#endif
	
	//----------------------------------------------
	#endif
}

int Point::vis = 0;

void Point::display(unsigned char pixel[4])
{
	#if EQUATION==DIRAC
	while (vis < 0) vis += 4;
	switch (vis % 4)
	{
		case 0: if (!defined(e0)) memset(pixel, 42, 4); else hsl(e0, pixel); break;
		case 1: if (!defined(e1)) memset(pixel, 42, 4); else hsl(e1, pixel); break;
		case 2: if (!defined(e2)) memset(pixel, 42, 4); else hsl(e2, pixel); break;
		case 3: if (!defined(e3)) memset(pixel, 42, 4); else hsl(e3, pixel); break;
	}
	#else
	while (vis < 0) vis += 2;
	switch (vis % 2)
	{
		case 0:
			if (!defined(e)) memset(pixel, 42, 4); else hsl(e, pixel);
			break;
		case 1: // impulse
		{
			hsl(cnum(px(), py()), pixel);
			break;
		}
	}
	#endif
}
