#include "GL_RM.h"
#include "GL_Image.h"

#ifdef TXDEBUG
#define DEBUG_TEXTURES(x) std::cerr << x << std::endl
#else
#define DEBUG_TEXTURES(x)
#endif

//----------------------------------------------------------------------------------------------------------------------
// Resource Management
//----------------------------------------------------------------------------------------------------------------------

void GL_RM::modified(const GL_Resource *r)
{
	auto i = resources.find(const_cast<GL_Resource*>(r));
	if (i == resources.end()){ assert(false); return; }
	
	for (auto &j : i->second)
	{
		j.modified = true;
	}
}

void GL_RM::deleted(const GL_Resource *r)
{
	auto i = resources.find(const_cast<GL_Resource*>(r));
	if (i == resources.end()){ assert(false); return; }
	
	orphans.insert(orphans.end(), i->second.begin(), i->second.end());
	resources.erase(const_cast<GL_Resource*>(r));
}

void GL_RM::clear_unused()
{
	for (auto i = resources.begin(); i != resources.end(); )
	{
		for (auto j = i->second.begin(); j != i->second.end(); )
		{
			if (!j->unused){ ++j; continue; }
			
			DEBUG_TEXTURES("Releasing tex_ID " << j->handle);
			context.delete_texture(j->handle);
			
			j = i->second.erase(j);
		}
		if (i->second.empty())
		{
			i->first->managers.erase(this);
			i = resources.erase(i);
		}
		else
		{
			++i;
		}
	}
	for (auto j = orphans.begin(); j != orphans.end(); ++j)
	{
		DEBUG_TEXTURES("Releasing tex_ID " << j->handle);
		context.delete_texture(j->handle);
	}
	orphans.clear();
}

GL_Handle GL_RM::upload_texture(const GL_Image &im)
{
	if (im.empty()) return 0;
	
	GL_CHECK;
	
	auto i = resources.find((GL_Resource*)(&im));
	if (i != resources.end())
	{
		assert(im.managers.count(this));

		// (1) check for exact matches
		
		for (auto &j : i->second)
		{
			if (!j.modified)
			{
				//DEBUG_TEXTURES("Texture (" << im.w << " x " << im.h << ") cached: " << j.handle);
				assert(j.w == im.w() && j.h == im.h());
				j.unused = false;
				glBindTexture(GL_TEXTURE_2D, j.handle);
				GL_CHECK;
				return j.handle;
			}
		}
		
		// (2) check for modified entries with the same size
		
		for (auto &j : i->second)
		{
			if (j.modified && j.w == im.w() && j.h == im.h())
			{
				//DEBUG_TEXTURES("Texture (" << im.w() << " x " << im.h() << ") reused: " << j.handle);
				
				assert(j.unused);
				j.unused = false;
				j.modified = false;
				
				// upload new data
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glBindTexture(GL_TEXTURE_2D, j.handle);
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, im.w(), im.h(), GL_RGBA, GL_UNSIGNED_BYTE, im.data().data());
				
				GL_CHECK;
				return j.handle;
			}
		}
	}

	// (3) add new item

	auto &v = resources[(GL_Resource*)(&im)];
	v.emplace_back();
	auto &j = v.back();
	im.managers.insert(this);
	
	glGenTextures(1, &j.handle);
	DEBUG_TEXTURES("Allocating tex_ID for " << &im << " (" << im.w() << " x " << im.h() << "): " << j.handle);
	if (!j.handle){ assert(false); return 0; }

	j.w = im.w(); j.h = im.h();
	j.unused = false;
	j.modified = false;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, j.handle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, im.w(), im.h(), 0, GL_RGBA, GL_UNSIGNED_BYTE, im.data().data());
	
	GL_CHECK;
	return j.handle;
}

void GL_RM::setup(bool repeat, bool interpolate, bool blend) const
{
	GL_CHECK;
	GLenum x = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, x);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, x);
	x = interpolate ? GL_LINEAR : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, x);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, x);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	x = blend ? GL_MODULATE : GL_REPLACE;
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, (GLfloat)x);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	GL_CHECK;
}


