#include "Graph.h"
#include "Utility/ThreadMap.h"
#include "Graphs/GL_Util.h"
#include "Point.h"
#include <GL/gl.h>
#include <thread>

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

Graph::Graph()
: m_animating(false)
, w(0), h(0)
, qz(2), tz(1)
, frame((size_t)-1)
, wave(new Wave)
{ }

Graph::~Graph()
{
	delete wave;
}

void Graph::viewport(int w_, int h_)
{
	w = w_; h = h_;
	glViewport(0, 0, w, h);
}

void Graph::draw() const
{
	try
	{
		update();
	}
	catch (const std::bad_alloc &)
	{
		#ifdef DEBUG
		std::cerr << "allocation failed!" << std::endl;
		#endif
		return;
	}
	catch (...)
	{
		#ifdef DEBUG
		std::cerr << "exception in update()!" << std::endl;
		#endif
		return;
	}
	if (im.empty()) return;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	GL_CHECK;
	glPixelZoom((GLfloat)w / im.w(), (GLfloat)h / im.h());
	glRasterPos2i(0, 0);
	glDrawPixels(im.w(), im.h(), GL_RGBA, GL_UNSIGNED_BYTE, im.data().data());
	glPixelZoom(1, 1);
	GL_CHECK;
}

//----------------------------------------------------------------------------------------------------------------------
// update and draw
//----------------------------------------------------------------------------------------------------------------------
#define BORDER Point::OVERLAP

void Graph::update() const
{
	static int nthreads = (int)std::thread::hardware_concurrency();
	int w = this->w / qz, h = this->h / qz;

	//------------------------------------------------------------------------------------------------------------------
	// (1) setup the info structs
	//------------------------------------------------------------------------------------------------------------------

	Task task;
	WorkLayer *layer = NULL;

	const int W = w + 2 * BORDER;
	const int chunk = std::max(1, h / (2*nthreads));
	const int space = (BORDER + chunk - 1) / chunk;

	//------------------------------------------------------------------------------------------------------------------
	// (2) calculation
	//------------------------------------------------------------------------------------------------------------------

	unsigned char *data = NULL;
	Point *ud = NULL;
	Point *ud0 = NULL;

	assert(wave->U.size() == wave->U0.size());

	++frame;
	if (frame == 0 || (int)im.w() != w || (int)im.h() != h)
	{
		// initial setup
		if (h < BORDER || w < BORDER)
		{
			im.redim(0, 0);
			wave->clear();
			return;
		}
		try
		{
			data = im.redim(w, h);
			size_t n = ((size_t)w + 2 * BORDER)*((size_t)h + 2 * BORDER);
			wave->U.resize(n);
			wave->U0.resize(n);
		}
		catch (...)
		{
			im.redim(0, 0);
			wave->clear();
		}
		if (im.empty()) return;

		ud = wave->U.data();
		ud0 = wave->U0.data();

		layer = new WorkLayer("init", &task, NULL);
		Point *p = ud + BORDER + W*BORDER;
		double hr = (double)h / (double)w;
		for (int i = 0; i < h; i += chunk)
		{
			int i1 = std::min(h, i + chunk);
			layer->add_unit([=]() mutable
			{
				for (; i < i1; ++i)
				{
					double y = (double)(h - 2 * i) / (double)w;
					for (int j = 0; j < w; ++j, ++p)
					{
						double x = (double)(w - 2 * j) / (double)w;
						p->init(x, y, hr);
					}
					p += 2 * BORDER;
				}
			});
			p += W*chunk;
		}
	}
	else
	{
		data = im.redim(w, h);

		ud = wave->U.data();
		ud0 = wave->U0.data();

		for (int t = 0; t < tz; ++t)
		{
			std::swap(ud, ud0);

			// U is the flat torus with BORDER points glued together
			// To avoid tons of modulo operations, we add BORDER-sized
			// borders on every side - the copying below is what buys 
			// the simpler evolve()

			#ifdef ZERO_BORDER
			layer = new WorkLayer("prepare", &task, layer, 0, -1);
			{
				layer->add_unit([=]()
				{
					memset(ud, 0, BORDER * W * sizeof(Point));
					memset(ud + (BORDER + h)*W, 0, BORDER * W * sizeof(Point));
				});
				Point *p = ud + W*BORDER;
				for (int i = 0; i < h; i += chunk)
				{
					int i1 = std::min(h, i + chunk);
					layer->add_unit([=]()
					{
						memset(p, 0, (i1 - i) * W * sizeof(Point));
					});
					p += W*chunk;
				}
			}
			#else
			{
				Point * const d = ud0;
				size_t ow = BORDER * sizeof(Point);
				layer = new WorkLayer("copy borders in U0", &task, layer, 0, -1);
				layer->add_unit([=]() { memcpy(d, d + h*W, W * ow); }); // top
				layer->add_unit([=]() { memcpy(d + (BORDER + h)*W, d + BORDER*W, W * ow); }); // bottom
				layer->add_unit([=]() { // left and right
					Point *l = d + BORDER*W;
					for (int y = 0; y < h; ++y, l += W)
					{
						memcpy(l, l + w, ow);
						memcpy(l + BORDER + w, l + BORDER, ow);
					}
				});
			}

			layer = new WorkLayer("prepare", &task, layer, 0, -1);
			{
				if (Point::MOD_OVERLAP > 0)
				{
					// need to clear even the borders because they will be added later
					layer->add_unit([=]()
					{
						memset(ud,              0, BORDER * W * sizeof(Point));
						memset(ud+(BORDER+h)*W, 0, BORDER * W * sizeof(Point));
					});

					// add a new layer so its striping can match the one below 
					layer = new WorkLayer("prepare2", &task, layer, 0, -1);
				}
				Point *p = ud + W*BORDER;
				for (int i = 0; i < h; i += chunk)
				{
					int i1 = std::min(h, i + chunk);
					layer->add_unit([=]()
					{
						memset(p, 0, (i1 - i) * W * sizeof(Point));
					});
					p += W*chunk;
				}
			}
			#endif
			layer = new WorkLayer("evolve", &task, layer, space, 2*space+1, -space);
			layer->set_cyclic();
			{
				Point *p = ud + BORDER + W*BORDER;
				Point *p0 = ud0 + BORDER + W*BORDER;
				for (int i = 0; i < h; i += chunk)
				{
					int i1 = std::min(h, i + chunk);
					layer->add_unit([=]() mutable
					{
						for (; i < i1; ++i)
						{
							for (auto *end = p + w; p != end; ++p, ++p0)
							{
								p->evolve(p0, W);
							}
							p += 2 * BORDER;
							p0 += 2 * BORDER;
						}
					});
					p += W*chunk;
					p0 += W*chunk;
				}
			}

			#ifndef ZERO_BORDER
			constexpr int m = Point::MOD_OVERLAP;
			if (m > 0)
			{
				// add border modification back to their original location
				assert(m <= BORDER);
				layer = new WorkLayer("copy borders in U", &task, layer, 1, -1);
				layer->add_unit([=]() // top
				{
					for (int i = BORDER-m; i < BORDER; ++i)
					{
						Point *p = ud + i*W;
						Point *q = ud + (i+h)*W;
						for (int j = 0; j < W; ++j) *q++ += *p++;
					}
				});

				// layer->space == 1 => next one is executed after the others are done
				layer->add_unit([=]() { // left and right
					Point *p = ud + BORDER*W;
					for (int y = 0; y < h; ++y, p += W)
					{
						for (int x = 0; x < m; ++x)
						{
							p[BORDER + x] += p[BORDER + w + x];
							p[BORDER - 1 + w - x] += p[BORDER - 1 - x];
						}
					}
				});

				layer->add_unit([=]() // bottom
				{
					Point *p = ud + (BORDER + h)*W;
					Point *q = ud +  BORDER*W;
					for (int i = 0; i < m; ++i)
					{
						for (int j = 0; j < W; ++j) *q++ += *p++;
					}
				});
			}
			#endif
		}

		if (tz & 1) wave->U.swap(wave->U0);
	}

	layer = new WorkLayer("visualize", &task, layer, 0, 1);
	Point *p = ud + BORDER + W*BORDER;
	for (int i = 0; i < h; i += chunk)
	{
		int i1 = std::min(h, i + chunk);
		layer->add_unit([=]() mutable
		{
			for (; i < i1; ++i)
			{
				for (auto *end = p+w; p != end; ++p, data += 4)
				{
					p->display(W, data);
				}
				p += 2 * BORDER;
			}
		});
		data += 4 * w * chunk;
		p += W*chunk;
	}

	task.run(nthreads);
}

