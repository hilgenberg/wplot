#pragma once
#include "Graphs/GL_Image.h"
class Recorder;

class Graph
{
public:
	Graph();
	~Graph();

	bool animating() const { return m_animating; }
	void animate(bool f) { m_animating = f; }
	void reset() { frame = (size_t)-1; } // start animation at t=0 again

	bool recording() const { return rec; }
	void record(bool f);

	void viewport(int w, int h);
	int  screen_w() const { return w; }
	int  screen_h() const { return h; }

	void draw() const;

	int  zoom() const { return qz; }
	void zoom(int z) { qz = z; if (qz < 1) qz = 1; }
	int  timezoom() const { return tz; }
	void timezoom(int z) { tz = z; if (tz < 1) tz = 1; }

private:
	void update() const;

	bool m_animating;
	int  qz; // quality reduction factor: generated image is w/qz x h/qz
	int  tz; // speedup factor: compute tz iterations per frame
	int  w, h;
	Recorder *rec;
	mutable GL_Image im;
	mutable struct Wave *wave;
	mutable size_t frame;
};
