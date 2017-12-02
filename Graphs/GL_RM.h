#pragma once
#include <GL/gl.h>
#include <set>
#include <vector>
#include <cassert>
#include "GL_Context.h"

/*----------------------------------------------------------------------------------------------------------------------
 Resource Handling:
 - there is one GL_RM per GL-context/window
 - every GL_RM keeps track of which resources it has uploaded to its context (and with which parameters, such as alpha)
 - every GL_Resource knows its managers so it can tell them about its modifications and deletion
 ---------------------------------------------------------------------------------------------------------------------*/

class GL_RM;

class GL_Resource ///< Something the Resource Manager can upload to OpenGL
{
protected:
	friend class GL_RM;
	
	GL_Resource(){ }
	
	GL_Resource(const GL_Resource &)
	{
		// do not copy the managers!
	}
	
	GL_Resource &operator=(const GL_Resource &)
	{
		// do not copy the managers!
		return *this;
	}

	inline ~GL_Resource();
	inline void modify();

private:
	mutable std::set<GL_RM*> managers;
};

typedef GLuint GL_Handle;

struct GL_Image;

class GL_RM ///< OpenGL Resource Manager
{
public:
	GL_RM(GL_Context context) : context(context), drawing(false){ }
	~GL_RM()
	{
		assert(!drawing);
		for (auto &i : resources)
		{
			i.first->managers.erase(this);
		}
	}
	
	bool matches(GL_Context c) const{ return context == c; }
	
	void start_drawing()
	{
		assert(!drawing);
		
		for (auto &i : resources)
		{
			for (auto &j : i.second)
			{
				j.unused = true;
			}
		}
		
		drawing = true;
	}
	
	void end_drawing()
	{
		assert(drawing);
		drawing = false;
		clear_unused();
	}
	
	GL_Handle upload_texture(const GL_Image &im);

	void setup(bool repeat, bool interpolate, bool blend) const;
	
	void modified(const GL_Resource *r);
	void deleted(const GL_Resource *r);

private:
	void clear_unused();
	GL_Context context;
	
	bool          drawing;

	struct ResourceInfo
	{
		GL_Handle handle;
		unsigned  w, h;
		bool      unused, modified;
	};
	std::map<GL_Resource*, std::vector<ResourceInfo>> resources;
	std::vector<ResourceInfo> orphans;
};


inline GL_Resource::~GL_Resource()
{
	for (GL_RM *m : managers) m->deleted(this);
}

inline void GL_Resource::modify()
{
	for (GL_RM *m : managers) m->modified(this);
}
