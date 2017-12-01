#pragma once
#include "GL_Image.h"
#include "GL_Graph.h"

class Graph : public CDocument
{
public:
	Graph::Graph() : m_animating(false), gl(*this), w(0), h(0)
	{
	}

	bool animating() const { return m_animating; }
	void animate(bool f) { m_animating = f; }
	void reset(); // start animation at t=0 again
	
	void viewport(int w, int h);
	int  screen_w() const { return w; }
	int  screen_h() const { return h; }

	void draw(GL_RM &rm) const;

private:
	bool m_animating;
	int  w, h;
	mutable GL_Graph gl;

	DECLARE_DYNCREATE(Graph)
};
