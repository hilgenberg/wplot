#pragma once
#include <GL/gl.h>
#include <cassert>
#include "../cnum.h"

void hsl(double h, double s, double l, unsigned char buf[4]);
void hsl(const cnum &z, unsigned char buf[4]);

#ifdef DEBUG
#include <GL/glu.h>
#include <iostream>

#define GL_CHECK do{\
	GLenum err = glGetError();\
	if (err) std::cerr << "glERROR: " << gluErrorString(err) << std::endl;\
	assert(err == GL_NO_ERROR);\
}while(0)

#else
#define GL_CHECK
#endif
