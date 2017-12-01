#include "Graph.h"
#include "GL_Context.h"
#include <GL/gl.h>
#include <cassert>
#include <algorithm>
#include <thread>

IMPLEMENT_DYNCREATE(Graph, CDocument)

void Graph::viewport(int w_, int h_)
{
	w = w_; h = h_;
	glViewport(0, 0, w, h);
}

void Graph::reset()
{
	gl.reset();
}

void Graph::draw(GL_RM &rm) const
{
	static int n_threads = (int)std::thread::hardware_concurrency();

	double q = 1.0;

	try
	{
		gl.update(n_threads, q);
	}
	catch (const std::bad_alloc &)
	{
		// ignore
		#ifdef DEBUG
		std::cerr << "allocation failed!" << std::endl;
		#endif
	}

	rm.start_drawing();
	try {

		GL_CHECK;

		glDisable(GL_DITHER);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		GL_CHECK;

		{
			GLboolean b;
			glGetBooleanv(GL_DOUBLEBUFFER, &b);
			GLenum buf = b ? GL_BACK : GL_FRONT;
			glDrawBuffer(buf);
			glReadBuffer(buf);
		}

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		GL_CHECK;

		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClearDepth(1.0);
		glDepthMask(GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_POINT_SMOOTH);

		glDisable(GL_DEPTH_TEST);
		gl.draw(rm);

		GL_CHECK;
	}
	catch (...)
	{
		rm.end_drawing();
		throw;
	}
	rm.end_drawing();
	GL_CHECK;
}
