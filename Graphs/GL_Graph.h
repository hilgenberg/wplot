#pragma once
#include "GL_Image.h"

class Graph;
class GL_RM;

class GL_Graph
{
public:
	GL_Graph(Graph &graph);
	~GL_Graph();

	virtual void update(int n_threads, double quality);
	virtual void draw(GL_RM &rm) const;

	void reset() { frame = (size_t)-1; }

private:
	Graph &graph;
	GL_Image im;
	struct Wave *wave; // data - moved into .cc
	size_t frame;
};
