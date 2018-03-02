#include "Point.h"
#include "Graphs/GL_Util.h"
#include "random.h"

static inline double sqr(double x) { return x*x; }

void Point::init(double x, double y, double Y)
{
	#ifdef DIRAC

	P2d v(38.0, 0.0);
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

	#elif defined(MAXWELL)
	
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
	
	#else
	
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
	double r = std::hypot(x, y);
	switch (4)
	{
		case 0: // nothing
			break;
		case 1: // mass at the bottom
			g.z = 1.0 / (2.0 - y);
			break;
		case 2: // mass at the left
			g.z = 1.0 / (2.0 - x); // 1/3 --> 1
			break;
		case 3: // mass at the right
			g.z = 1.0 / (2.0 + x); // 1 --> 1/3
			break;

		case 4: // black hole
		{
			const double rh = .15;
			g.z = r < rh ? 1.0 : sqr(rh / r);
			break;
		}
		case 5: // rotating black hole
		{
			const double rh = .15, f = r < rh ? 1.0 : rh / r;
			g.set(0.05*f*y / r, -0.05*f*x / r, f*f);
			break;
		}
		case 6: // only rotating
		{
			g.set(0.1*y / r, -0.1*x / r, 0.0);
			break;
		}
		case 7: // dark energy at the bottom
			g.z = -2.0 / sqr(2.0 - y); // => sqrt(1-g00) > 1
			break;
		case 8: // chaos
			g.set(0.1*normal_rand(), 0.1*normal_rand(), 0.1*normal_rand());
			break;
		case 9: // constant (gxx,gyy)
			g.y = 0.4;
			break;
	}

	// move the sqrt out of evolve() for now (though g.z > 1 could be
	// interesting later (fix precision problems first)!)
	g.z = sqrt(1.0 - g.z);
}

// p[+dx], p[-dy], p[-dx+dy] etc are the neighbours of p[0]
#define dx 1
#define dy Y

// differentials, first order:
#define D1L_(f,v) (P[0].f - P[-d##v].f) // left/lower side
#define D1R_(f,v) (P[d##v].f - P[0].f)  // right/upper side
#define D1L(f,v)  (D1L_(f,v)*(1.0-g.v))
#define D1R(f,v)  (D1R_(f,v)*(1.0+g.v))
#define D1(f,v)  ((D1L(f,v)+D1R(f,v))*0.5) // probably needs roots on the factors or something

// second order: D2 = D1R - D1L = 
#define D2(f,v)  ((P[-d##v].f*(1.0-g.v) - P[0].f) + (P[d##v].f*(1.0+g.v) - P[0].f))
#define LAPLACE(f) (D2(f,x)+D2(f,y))

#define PTL(f,v) do { double p = sp(-ix(D1L(f, v)), P->f)*dt; m += p; this[-d##v].m -= p; } while (0)
#define PTR(f,v) do { double p = sp(-ix(D1R(f, v)), P->f)*dt; m -= p; this[ d##v].m += p; } while (0)

void Point::evolve(const Point *P, const int Y)
{
	g = P->g; // static gravity for now

	#ifdef DIRAC //---------------------------------------
	constexpr double dt = 0.1;
	constexpr double eps = 1.0;// / (double)w;
	static constexpr cnum I(0.0, 1.0);
	#if 1
	static const cnum c03 = -I, c02 = -1.0;
	static const cnum c12 = 0, c13 = 0.0;
	static const cnum c21 = 0, c20 = -1.0;
	static const cnum c30 = I, c31 = 0.0;
	static constexpr double m0 = 1.0, m1 = 0, m2 = 0, m3 = 0;
	#else
	static const cnum c03 = -I, c02 = -1.0;
	static const cnum c12 = I, c13 = 0.0;
	static const cnum c21 = -I, c20 = -1.0;
	static const cnum c30 = I, c31 = 1.0;
	static constexpr double m0 = 1.0, m1 = 1.0, m2 = 1.0, m3 = 1.0;
	#endif
	double f = dt * g.z;
	e0 += P->e0 + f*(c03*D1(e3, x) + c02*D1(e2, y) - m0*ix(P->e0));
	e1 += P->e1 + f*(c12*D1(e2, x) + c13*D1(e3, y) - m1*ix(P->e1));
	e2 += P->e2 + f*(c21*D1(e1, x) + c20*D1(e0, y) - m2*ix(P->e2));
	e3 += P->e3 + f*(c30*D1(e0, x) + c31*D1(e1, y) - m3*ix(P->e3));

	#else //----------------------------------------------
	constexpr double dt = 0.1;
	constexpr double eps = 1.0;// / (double)w;

	// start with last iteration's value (but allow
	// neighbours to modify our e)
	e += P->e;

	#ifdef MAXWELL

	de = p->de + dt*LAPLACE(e) / (eps*eps);
	
	#else // Klein-Gordon
	
	de = P->de + dt*(LAPLACE(e)/(eps*eps) - P->e);
	
	#endif

	cnum d = dt*de*g.z;

	e += d;

	#endif // DIRAC
}

int Point::vis = 0;

void Point::display(const int Y, unsigned char pixel[4])
{
	#ifdef DIRAC
	switch (vis % 4)
	{
		case 0: if (!defined(e0)) memset(pixel, 42, 4); else hsl(e0, pixel); break;
		case 1: if (!defined(e1)) memset(pixel, 42, 4); else hsl(e1, pixel); break;
		case 2: if (!defined(e2)) memset(pixel, 42, 4); else hsl(e2, pixel); break;
		case 3: if (!defined(e3)) memset(pixel, 42, 4); else hsl(e3, pixel); break;
	}
	#else
	switch (vis % 2)
	{
		case 0:
			if (!defined(e)) memset(pixel, 42, 4); else hsl(e, pixel);
			break;
		case 1: // impulse
		{
			const Point *P = this;
			hsl(cnum(sp(-ix(D1L(e, x)), e), sp(-ix(D1L(e, y)), e)), pixel);
			break;
		}
	}
	#endif
}
