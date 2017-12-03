#include "Point.h"
#include "Graphs/GL_Util.h"
#include "random.h"

//#define MAXWELL

void Point::display(unsigned char pixel[4])
{
	hsl(e, pixel);
}

static inline double sqr(double x) { return x*x; }
void Point::init(double x, double y)
{
	#ifdef MAXWELL
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
	switch (8)
	{
		case 0: // nothing
			g.set(0.0, 0.0, 0.0);
			break;
		case 1: // mass at the bottom
			g.set(0.0, 0.0, 1.0 / sqr(y - 2.0));
			break;
		case 2: // mass at the right
			g.set(0.0, 0.0, 1.0 / sqr(2.0 - x));
			break;
		case 3: // mass at the left
			g.set(0.0, 0.0, 1.0 / sqr(x + 2.0));
			break;

		case 4: // black hole
		{
			double r = 2.0*(x*x + y*y);
			const double rh = .05;
			g.set(0.0, 0.0, r < rh ? 1.0 : sqr(rh / r));
			break;
		}
		case 5: // rotating black hole
		{
			double r = 2.0*(x*x + y*y);
			const double rh = .05, f = r < rh ? 1.0 : rh / r;
			g.set(0.1*f*y / r, -0.1*f*x / r, f*f);
			break;
		}
		case 6: // only rotating
		{
			double r = 2.0*(x*x + y*y);
			const double rh = .05, f = r < rh ? 1.0 : rh / r;
			g.set(0.01*y / r, -0.01*x / r, 0.0);
			break;
		}
		case 7: // dark energy at the bottom
			g.set(0.0, 0.0, -1.0 / sqr(y - 2.0));
			break;
		case 8: // chaos
			g.set(0.1*normal_rand(), 0.1*normal_rand(), 0.1*normal_rand());
			break;
	}
}

// p[+X], p[-Y], p[-X+Y] etc are the neighbours (Y is passed as argument)
#define X 1

// differentials: first order is ambiguous
#define DL(f,v) ((p[0].f - p[-v].f)/eps) // left side
#define DR(f,v) ((p[v].f - p[0].f)/eps)  // right side

// second order is not: (DR(f,v) - DL(f,v)) / eps = 
#define D2(f,v)  ((p[v].f - p[0].f + p[-v].f - p[0].f) / (eps*eps))

#define LAPLACE(f) ((p[X].f - p[0].f + (p[-X].f - p[0].f) + (p[Y].f - p[0].f) + (p[-Y].f - p[0].f)) / (eps*eps))
#define LAPLACEG(f) (p[-X].f*(1.0-g.x) - p[0].f + (p[X].f*(1.0+g.x) - p[0].f)+ \
					 (p[-Y].f*(1.0-g.y) - p[0].f) + (p[Y].f*(1.0+g.y) - p[0].f))/(eps*eps)

// curl is also ok if f is 2d
#define CURL(f) ((p[+Y].f.x-p[-Y].f.x-p[+X].f.y+p[-X].f.y)/eps)
#define CCURL(f) ((p[+Y].f.real()-p[-Y].f.real()-p[+X].f.imag()+p[-X].f.imag())/eps)

// divergence is weird again, but maybe like this:
#define DIV(f) ((p[X].f.x - p[-X].f.x + p[Y].f.y - p[-Y].f.y)/eps)

void Point::evolve(const Point *p, const int Y)
{
	g = p->g; // static gravity for now
	e += p->e; // start with last iteration's value (but allow neighbours to modify our e)

	//de = p->de + 0.1*LAPLACEG(e)*dt;
	//de = p->de - 0.1*p->e*dt;

	#ifdef MAXWELL
	constexpr double dt = 0.1;
	const double eps = 1.0;// / (double)w;
	de = p->de + dt*LAPLACEG(e);
	#else // Klein-Gordon
	constexpr double dt = 0.1;
	const double eps = 1.0;// / (double)w;
	de = p->de + dt*(LAPLACEG(e) - p->e);
	#endif

	cnum d = dt*de*sqrt_(1.0 - g.z); // one dt is already in de

	#if 1
	e += d;
	#else
	// conserve |e| to work against rounding errors
	double va = abs(p[X].e);
	double vb = abs(p[-X].e);
	double vc = abs(p[Y].e);
	double vd = abs(p[-Y].e);
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
		this[X].e -= p[X].e * dr;
		this[-X].e -= p[-X].e * dr;
		this[Y].e -= p[Y].e * dr;
		this[-Y].e -= p[-Y].e * dr;
	}
	#endif

	/*double r = abs(e);
	if (r > 1.0) e /= r;*/
}
