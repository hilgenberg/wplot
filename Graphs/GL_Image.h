#pragma once
#include "GL_RM.h"
#include <vector>

struct GL_Image : public GL_Resource
{
	GL_Image() : _w(0), _h(0){ }
	GL_Image(const GL_Image &i) = delete;
	GL_Image &operator=(const GL_Image &x) = delete;

	bool operator==(const GL_Image &x) const
	{
		return _w == x._w && _h == x._h && _data == x._data;
	}

	inline void clear() { redim(0, 0); }
	
	unsigned char *redim(unsigned w, unsigned h)
	{
		_w = w; _h = h;
		_data.resize(_w * _h * 4);
		modify(); // even if w==w_ and h==h_ !
		return _data.data();
	}
	
	const std::vector<unsigned char> &data() const { return _data; }

	unsigned w() const{ return _w; }
	unsigned h() const{ return _h; }
	bool empty() const{ return _w == 0 || _h == 0; }
	
private:
	unsigned                   _w, _h;   // width and height
	std::vector<unsigned char> _data;    // rgba, size = 4*w*h or empty for patterns
};
