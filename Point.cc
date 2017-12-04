#include "Point.h"
#include "Graphs/GL_Util.h"
#include "random.h"

void Point::display(unsigned char pixel[4])
{
	#ifdef DIRAC
	hsl(e0, pixel);
	#else
	hsl(e, pixel);
	#endif
}

static inline double sqr(double x) { return x*x; }
void Point::init(double x, double y, double Y)
{
	#ifdef DIRAC

	P2d v(180.0, 0.0);
	double r = 2.0*(x*x + y*y);
	if (r < 1.0)
	{
		double s = sin(v.x*x + v.y*y), c = cos(v.x*x + v.y*y);
		double f = 6.0 * exp(-1.0 / (1.0 - r));
		e0 = f*cnum(c, -s);
		e1 = 0.0;
		e2 = f*v.y*cnum(c, -s);
		e3 = f*v.x*cnum(c, -s);
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
	if (r < 1.0)
	{
		double s = sin(v.x*x + v.y*y), c = cos(v.x*x + v.y*y);
		double f = 6.0 * exp(-1.0 / (1.0 - r));
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
	switch (1)
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
#define D1L_(f,v) (p[0].f - p[-d##v].f) // left/lower side
#define D1R_(f,v) (p[d##v].f - p[0].f)  // right/upper side
#define D1L(f,v)  (D1L_(f,v)*(1.0-g.v))
#define D1R(f,v)  (D1R_(f,v)*(1.0+g.v))
#define D1(f,v)  ((D1L(f,v)+D1R(f,v))*0.5) // probably needs roots on the factors or something

// second order: D2 = D1R - D1L = 
#define D2(f,v)  ((p[-d##v].f*(1.0-g.v) - p[0].f) + (p[d##v].f*(1.0+g.v) - p[0].f))
#define LAPLACE(f) (D2(f,x)+D2(f,y))

void Point::evolve(const Point *p, const int Y)
{
	constexpr double dt = 0.1;
	constexpr double eps = 1.0;// / (double)w;

	g = p->g;  // static gravity for now

	#ifdef DIRAC //---------------------------------------
	static constexpr cnum I(0.0, 1.0);
	double f = dt * sqrt(g.z); // first order equation

	e0 += p->e0 + f*(-D1(e3, x) + ix(D1(e3, y)) - ix(p->e0));
	e1 += p->e1 + f*(-D1(e2, x) - ix(D1(e2, y)) - ix(p->e1));
	e2 += p->e2 + f*(-D1(e1, x) + ix(D1(e1, y)) + ix(p->e2));
	e3 += p->e3 + f*(-D1(e0, x) - ix(D1(e0, y)) + ix(p->e3));

	#else //----------------------------------------------

	// start with last iteration's value (but allow
	// neighbours to modify our e)
	e += p->e;

	#ifdef MAXWELL

	de = p->de + dt*LAPLACE(e) / (eps*eps);
	
	#else // Klein-Gordon
	
	de = p->de + dt*(LAPLACE(e)/(eps*eps) - p->e);
	
	#endif

	cnum d = dt*de*g.z;

	#if 1
	e += d;
	#else
	// conserve |e| to work against rounding errors
	double va = abs(p[ dx].e);
	double vb = abs(p[-dx].e);
	double vc = abs(p[ dy].e);
	double vd = abs(p[-dy].e);
	double v = va + vb + vc + vd;

	double r0 = abs(p->e), r = abs(p->e + d);
	double dr = r - r0;

	if (abs(dr) > 1e-40 && dr > v)
	{
		d *= v / dr;
		dr = v;
	}
	if (v > 1e-40)
	{
		dr /= v;
		e += d;
		this[ dx].e -= p[ dx].e * dr;
		this[-dx].e -= p[-dx].e * dr;
		this[ dy].e -= p[ dy].e * dr;
		this[-dy].e -= p[-dy].e * dr;
	}
	#endif
	#endif // DIRAC

	/*double r = abs(e);
	if (r > 1.0) e /= r;*/
}
