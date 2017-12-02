#include "Point.h"
#include "Graphs/GL_Util.h"

static inline double arg(double y, double x)
{
	double f = std::max(abs(x), abs(y));
	if (f < 1e-40) return 0.0;
	double a = std::min(abs(x), abs(y)) / f;
	double s = a * a;
	double r = ((-0.0464964749 * s + 0.15931422) * s - 0.327622764) * s * a + a;
	if (abs(y) > abs(x)) r = M_PI_2 - r;
	if (x < 0) r = M_PI - r;
	if (y < 0) r = -r;
	return r * 0.5*M_1_PI; // range [-0.5, 0.5]
}

void Point::display(unsigned char pixel[4])
{
	double x = e.real(), y = e.imag();
	double v = hypot(x, y);
	hsl(arg(y, x) + 1.0, 1.0, std::min(v*0.3, 0.9), pixel);
}

void Point::init(double x, double y)
{
	double r = 2.0*(x*x + y*y);
	if (r < 1.0)
	{
		double s = sin(38 * x), c = cos(38 * x);
		double f = 6.0 * exp(-1.0 / (1.0 - r));
		e  = cnum(s,  c) * f;
		de = cnum(c, -s) * f;
	}
	else
	{
		clear();
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

// curl is also ok if f is 2d
#define CURL(f) ((p[+Y].f.x-p[-Y].f.x-p[+X].f.y+p[-X].f.y)/eps)
#define CCURL(f) ((p[+Y].f.real()-p[-Y].f.real()-p[+X].f.imag()+p[-X].f.imag())/eps)

// divergence is weird again, but maybe like this:
#define DIV(f) ((p[X].f.x - p[-X].f.x + p[Y].f.y - p[-Y].f.y)/eps)

void Point::evolve(const Point *p, const int Y)
{
	constexpr double dt = 0.1;
	const double eps = 1.0;// / (double)w;

	switch (0)
	{
		case 0: // Klein-Gorden-like
		{
			assert(e == 0.0);
			assert(de == 0.0);

			de = p->de + (0.9*LAPLACE(e) - p->e)*dt;

			// conserve |e| to work against rounding errors
			cnum d = dt*de;

			double va = abs(p[ X].e);
			double vb = abs(p[-X].e);
			double vc = abs(p[ Y].e);
			double vd = abs(p[-Y].e);
			double v = va + vb + vc + vd;

			double r0 = abs(p->e), r = abs(p->e + d);
			double dr = r - r0;

			e += p->e;
			if (abs(dr) > 1e-40 && dr > v)
			{
				d *= v / dr;
				dr = v;
			}
			if (v > 1e-40)
			{
				e += d;
				this[ X].e -= p[ X].e * dr / v;
				this[-X].e -= p[-X].e * dr / v;
				this[ Y].e -= p[ Y].e * dr / v;
				this[-Y].e -= p[-Y].e * dr / v;
			}

			break;
		}
		case 1: // Schrödinger-like
			e = p->e + dt*LAPLACE(e)*cnum(0, 1.0);
			break;
		case 2:
			de = p->de + 0.01*dt*cnum(CCURL(e), CCURL(de));// (0.01*LAPLACE(e) - p->e)*dt;
			e = p->e + p->de*dt;
			break;
	}

	/*double r = abs(e);
	if (r > 1.0) e /= r;*/
}
