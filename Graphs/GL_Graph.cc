#include "GL_Graph.h"
#include "../Utility/ThreadMap.h"
#include "GL_Util.h"
#include "GL_RM.h"
#include "Graph.h"
#include "../cnum.h"
#include <vector>
#include <algorithm>

struct Point
{
	cnum e, de;
	//cnum g, dg;

	void clear()
	{
		memset(this, 0, sizeof(Point));
	}

	inline void init(double x, double y)
	{
		double r = 8.0*(x*x+y*y);
		if (r < 1.0 - 1e-1)
		{
			e = 6.0 * cnum(sin(180 * x), cos(180 * x)) * exp(-1.0/(1.0-r));
		}
		else
		{
			e = 0;
		}
		de = 0.0;
	}

	inline void evolve(const Point *p, int dy)
	{
		constexpr double dt = 0.01;
		const double eps = 0.1;// / (double)w;
		const int X = 1, Y = dy; // p[+x], p[-y], p[-x+2*y] etc are the neighbours

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
		#define DIV(f) ((p[X].f.x-p[-X].f.x+p[Y].f.y-p[-Y].f.y)/eps)

		//de = p->de + (0.1*LAPLACE(e) - p->e)*dt; // Klein-Gorden-like
		e = p->e + dt*LAPLACE(e)*cnum(0,1.0); // Schrödinger
		double r = abs(e);
		if (r > 1.0) e /= r;
		//de = p->de + 0.01*dt*cnum(CCURL(e), CCURL(de));// (0.01*LAPLACE(e) - p->e)*dt;
		//e = p->e + p->de*dt;
		//q->e = p->e + D2(e, X) + D2(e, Y);
		//q->e = p->e - 0.1*CCURL(e);
		//*p = *p0;
		//q->e = (p[-1].e + p->e + p[1].e) / 3.0;
		#undef DL
		#undef DR
		#undef D2
	}
};

struct Wave
{
	// after update, the current state is always in U
	std::vector<Point> U, U0;

	void clear()
	{
		std::vector<Point>().swap(U);
		std::vector<Point>().swap(U0);
	}
};

//----------------------------------------------------------------------------------------------------------------------
// update worker: fill in the area [x1,x2) x [y1,y2)
//----------------------------------------------------------------------------------------------------------------------

static const int OVERLAP = 1; // how many neighbouring points does a point's calculation need?

static void init(Point *p, int w, int h, int i, int i1)
{
	for (; i < i1; ++i)
	{
		double y = (double)(h - 2*i) / (double)w;
		for (int j = 0; j < w; ++j, ++p)
		{
			double x = (double)(w - 2 * j) / (double)w;
			p->init(x, y);
		}
		p += 2*OVERLAP;
	}
}

static void evolve(Point *q, const Point *p, int w, int h, int i, int i1)
{
	const int dy = w + 2 * OVERLAP;
	
	for (; i < i1; ++i)
	{
		for (int j = 0; j < w; ++j, ++p, ++q)
		{
			q->evolve(p, dy);
		}
		p += 2 * OVERLAP; // skip the borders
		q += 2 * OVERLAP;
	}
}

typedef unsigned char byte;
static void hsl(double h, double s, double l, unsigned char buf[4])
{
	if (s < 0.0) s = 0.0; else if (s > 1.0) s = 1.0;
	if (l < 0.0) l = 0.0; else if (l > 1.0) l = 1.0;
	l *= 255.0;
	double q = l + s * (l < 127.5 ? l : 255.0 - l);
	double p = l + l - q;
	while (h < 0.0) ++h;
	double ti, tr = modf(6.0*h, &ti);
	int i = (int)ti;
	if (tr < 0.0)
	{
		++tr; i += 5; // tr + i == 6h + 6 now
		i %= 6;
		if (i < 0) i += 6;
	}
	else
	{
		assert(i >= 0);
		i %= 6;
	}
	assert(tr >= 0.0);
	assert(i >= 0 && i < 6);
	switch (i)
	{
		case 0: *buf++ = (byte)q; *buf++ = (byte)(p + (q - p) * tr); *buf++ = (byte)p; break;
		case 1: *buf++ = (byte)(q - (q - p) * tr); *buf++ = (byte)q; *buf++ = (byte)p; break;
		case 2: *buf++ = (byte)p; *buf++ = (byte)q; *buf++ = (byte)(p + (q - p) * tr); break;
		case 3: *buf++ = (byte)p; *buf++ = (byte)(q - (q - p) * tr); *buf++ = (byte)q; break;
		case 4: *buf++ = (byte)(p + (q - p) * tr); *buf++ = (byte)p; *buf++ = (byte)q; break;
		case 5: *buf++ = (byte)q; *buf++ = (byte)p; *buf++ = (byte)(q - (q - p) * tr); break;
	}

	*buf = 255;
}

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

static void transfer(const Point *p, byte *d, int w, int h, int i, int i1)
{
	for (; i < i1; ++i)
	{
		for (int j = 0; j < w; ++j, ++p, d += 4)
		{
			double x = p->e.real(), y = p->e.imag();
			double v = hypot(x, y);
			hsl(arg(y,x) + 1.0, 1.0, v*0.5, (byte*)d);
		}
		p += 2*OVERLAP;
	}
}

//----------------------------------------------------------------------------------------------------------------------
// update and draw
//----------------------------------------------------------------------------------------------------------------------

GL_Graph::GL_Graph(Graph &graph)
: graph(graph)
, frame((size_t)-1)
, wave(new Wave)
{ }

GL_Graph::~GL_Graph()
{
	delete wave;
}

void GL_Graph::update(int nthreads, double quality)
{
	auto &U = wave->U, &U0 = wave->U0;
	std::swap(U, U0);
	assert(U.size() == U0.size());

	//------------------------------------------------------------------------------------------------------------------
	// (1) setup the info structs
	//------------------------------------------------------------------------------------------------------------------

	Task task;
	WorkLayer *layer = NULL;

	int w = graph.screen_w(), W = w + 2 * OVERLAP;
	int h = graph.screen_h();
	int chunk = std::max(1, (h + nthreads - 1) / nthreads);

	//------------------------------------------------------------------------------------------------------------------
	// (2) calculation
	//------------------------------------------------------------------------------------------------------------------
	
	unsigned char *data = NULL;

	++frame;
	if (frame == 0 || (int)im.w() != w || (int)im.h() != h)
	{
		// initial setup
		if (h < OVERLAP || w < OVERLAP)
		{
			im.redim(0, 0);
			wave->clear();
			return;
		}
		try
		{
			data = im.redim(w, h);
			size_t n = ((size_t)w + 2 * OVERLAP)*((size_t)h + 2 * OVERLAP);
			U.resize(n);
			U0.resize(n);
		}
		catch (...)
		{
			im.redim(0, 0);
			wave->clear();
		}
		if (im.empty()) return;

		layer = new WorkLayer("init", &task, NULL);
		Point *p = U.data() + OVERLAP + W*OVERLAP;
		for (int i = 0; i < h; i += chunk)
		{
			int i1 = std::min(h, i + chunk);
			layer->add_unit([=]() { ::init(p, w, h, i, i1); });
			p += W*chunk;
		}
	}
	else
	{
		data = im.redim(w, h); // mark as changed

		// U is the flat torus with OVERLAP points glued together
		// To avoid tons of modulo operations, we add OVERLAP-sized
		// borders on every side - the copying below is what buys 
		// the simpler evolve()
		{
			Point * const d = U0.data();
			size_t ow = OVERLAP * sizeof(Point);
			layer = new WorkLayer("copy borders", &task, NULL);
			layer->add_unit([=]() { memcpy(d, d + h*W, W * ow); });
			layer->add_unit([=]() { memcpy(d + (OVERLAP + h)*W, d + OVERLAP*W, W * ow); });
			layer->add_unit([=]() {
				Point *l = d + OVERLAP*W;
				for (int y = 0; y < h; ++y, l += W)
				{
					memcpy(l, l + w, ow);
					memcpy(l + OVERLAP + w, l + OVERLAP, ow);
				}
			});
		}

		layer = new WorkLayer("evolve", &task, layer, 0, -1);
		Point *p  = U.data() + OVERLAP + W*OVERLAP;
		Point *p0 = U0.data() + OVERLAP + W*OVERLAP;
		for (int i = 0; i < h; i += chunk)
		{
			int i1 = std::min(h, i + chunk);
			layer->add_unit([=]() { ::evolve(p, p0, w, h, i, i1); });
			p  += W*chunk;
			p0 += W*chunk;
		}
	}

	layer = new WorkLayer("visualize", &task, layer, 0, 1);
	Point *p = U.data() + OVERLAP + W*OVERLAP;
	for (int i = 0; i < h; i += chunk)
	{
		int i1 = std::min(h, i + chunk);
		layer->add_unit([=]() { ::transfer(p, data, w, h, i, i1); });
		data += 4 * w * chunk;
		p += W*chunk;
	}

	task.run(nthreads);
}

void GL_Graph::draw(GL_RM &rm) const
{
	if (im.empty()) return;
	
	//------------------------------------------------------------------------------------------------------------------
	// setup light and textures
	//------------------------------------------------------------------------------------------------------------------
	
	glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	glActiveTexture(GL_TEXTURE0);
	glClientActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	
	rm.upload_texture(im);
	rm.setup(true, false, false);
	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);

	//------------------------------------------------------------------------------------------------------------------
	// draw triangles
	//------------------------------------------------------------------------------------------------------------------
	glBegin(GL_QUADS);
	glNormal3f  (0.0f, 0.0f, 1.0f);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 0.0f);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}
