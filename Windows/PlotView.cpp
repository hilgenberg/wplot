#include "stdafx.h"
#include "PlotView.h"
#include "MainWindow.h"
#include "CPlotApp.h"
#include "../Graphs/Graph.h"

#include "gl/gl.h"
#include "gl/glu.h"

IMPLEMENT_DYNCREATE(PlotView, CWnd)
BEGIN_MESSAGE_MAP(PlotView, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_CONTEXTMENU()

	ON_WM_LBUTTONDOWN()
	ON_WM_MBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()

	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

PlotView::PlotView()
: dc(NULL)
, tnf(-1.0)
, last_frame(-1.0)
, last_key(-1.0)
, rm(NULL)
, nums_on(0)
, timer(1.0/60.0)
, in_resize(false)
{
	mb.all = 0;
	arrows.all = 0;
	memset(inertia, 0, 3 * sizeof(double));
	memset(nums, 0, 10 * sizeof(bool));
	timer.callback = [this]() { Invalidate(); };
}

PlotView::~PlotView()
{
	timer.stop();
	delete rm;
	delete dc;
}

Graph &PlotView::graph() const
{
	MainWindow *w = (MainWindow*)GetParentFrame();
	assert(w);
	return w->graph;
}

UINT PlotView::OnGetDlgCode()
{
	return CWnd::OnGetDlgCode() | DLGC_WANTARROWS | DLGC_WANTCHARS;
}

BOOL PlotView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CHILD | WS_GROUP | WS_TABSTOP;
	return CWnd::PreCreateWindow(cs);
}

BOOL PlotView::Create(const RECT &rect, CWnd *parent, UINT ID)
{
	static bool init = false;
	if (!init)
	{
		WNDCLASS wndcls; memset(&wndcls, 0, sizeof(WNDCLASS));
		wndcls.style = CS_DBLCLKS;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.hInstance = AfxGetInstanceHandle();
		wndcls.hCursor = theApp.LoadStandardCursor(IDC_ARROW);
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = _T("PlotView");
		wndcls.hbrBackground = NULL;
		if (!AfxRegisterClass(&wndcls)) throw std::runtime_error("AfxRegisterClass(PlotView) failed");
		init = true;
	}

	return CWnd::Create(_T("PlotView"), NULL, WS_CHILD | WS_TABSTOP, rect, parent, ID);
}

int PlotView::OnCreate(LPCREATESTRUCT cs)
{
	if (CWnd::OnCreate(cs) == -1) return -1;

	dc = new CClientDC(this);
	HDC hdc = dc->GetSafeHdc();

	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		24,                 // color depth
		0, 0, 0, 0, 0, 0,   // color bits ignored
		0,                  // no alpha buffer
		0,                  // shift bit ignored
		0,                  // no accumulation buffer
		0, 0, 0, 0,         // accum bits ignored
		32,                 // z-buffer depth
		0,                  // no stencil buffer
		0,                  // no auxiliary buffer
		PFD_MAIN_PLANE,     // main layer
		0,                  // reserved
		0, 0, 0             // layer masks ignored
	};
	int pf = ChoosePixelFormat(hdc, &pfd);
	if (!pf || !SetPixelFormat(hdc, pf, &pfd)) return -1;

	pf = ::GetPixelFormat(hdc);
	::DescribePixelFormat(hdc, pf, sizeof(pfd), &pfd);

	HGLRC ctx = wglCreateContext(hdc);
	wglMakeCurrent(hdc, ctx);

	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		CStringA s; s.Format("GLEW init failed: %s", glewGetErrorString(err));
		AfxMessageBox(CString(s));
		return -1;
	}

	rm = new GL_RM(GL_Context(ctx));

	GetClientRect(&bounds);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);

	return 0;
}

void PlotView::OnDestroy()
{
	HGLRC ctx = ::wglGetCurrentContext();
	::wglMakeCurrent(NULL, NULL);
	if (ctx) ::wglDeleteContext(ctx);

	delete dc; dc = NULL;

	CWnd::OnDestroy();
}

//----------------------------------------------------------------------------------------------------------

BOOL PlotView::OnEraseBkgnd(CDC *dc)
{
	return false;
}

bool PlotView::animating()
{
	bool anim = graph().animating();
	if (anim)
	{
		if (!timer.running()) timer.start();
	}
	else
	{
		if (timer.running()) timer.stop();
	}
	return anim;
}

void PlotView::OnPaint()
{
	if (last_frame <= 0.0) reshape();
	last_frame = now();

	CPaintDC dc(this);

	if (animating())
	{
		handleArrows();
	}

	GL_CHECK;
	graph().draw(*rm);
	GL_CHECK;

	glFinish();
	SwapBuffers(wglGetCurrentDC());
}

void PlotView::OnSize(UINT nType, int w, int h)
{
	CWnd::OnSize(nType, w, h);
	GetClientRect(&bounds);
	reshape();
	in_resize = true;
}

void PlotView::reshape()
{
	graph().viewport(bounds.Width(), bounds.Height());
}

//----------------------------------------------------------------------------------------------------------
//  Mouse
//----------------------------------------------------------------------------------------------------------

void PlotView::OnButtonDown(int i, CPoint p)
{
	m0 = p;
	if (!mb.all)
	{
		SetCapture();
		// StartAnimation();
	}
	mb.all |= 1 << i;
}
void PlotView::OnLButtonDown(UINT flags, CPoint p) { OnButtonDown(0, p); }
void PlotView::OnMButtonDown(UINT flags, CPoint p) { OnButtonDown(1, p); }
void PlotView::OnRButtonDown(UINT flags, CPoint p) { OnButtonDown(2, p); }

void PlotView::OnButtonUp(int i, CPoint p)
{
	mb.all &= ~(1 << i);
	if (!mb.all)
	{
		ReleaseCapture();
		// EndAnimation();
	}
}
void PlotView::OnLButtonUp(UINT flags, CPoint p) { OnButtonUp(0, p); }
void PlotView::OnMButtonUp(UINT flags, CPoint p) { OnButtonUp(1, p); }
void PlotView::OnRButtonUp(UINT flags, CPoint p) { OnButtonUp(2, p); }

/*static void ClearQueue(NSEvent *e, NSUInteger mask, std::function<void(NSEvent*)> handler)
{
	while (e)
	{
		handler(e);
		e = [NSApp nextEventMatchingMask : mask untilDate : nil inMode : NSDefaultRunLoopMode dequeue : YES];
	}
}*/

enum
{
	CONTROL = 1,
	ALT     = 2,
	SHIFT   = 4
};

//----------------------------------------------------------------------------------------------------------------------
//  keyboard
//----------------------------------------------------------------------------------------------------------------------

static inline void accel(double &inertia, double dt)
{
	inertia += 0.4*std::max(1.0, dt) + inertia*0.4;
	inertia = std::min(inertia, 20.0);
}

void PlotView::handleArrows()
{
	int dx = 0, dy = 0, dz = 0;
	if (arrows.left)  --dx;
	if (arrows.right) ++dx;
	if (arrows.up)    --dy;
	if (arrows.down)  ++dy;
	if (arrows.plus)  ++dz;
	if (arrows.minus) --dz;

	double t0 = now();
	double dt = t0 - last_key;
	double dt0 = 1.0 / 60.0;
	last_key = t0;
	double scale = (dt0 > 0 ? std::min(dt / dt0, 5.0) : 1.0);

	if (dx) accel(inertia[0], scale);
	if (dy) accel(inertia[1], scale);
	if (dz) accel(inertia[2], scale);

	if (nums_on)
	{
		/*for (int i = 0; i < 10; ++i)
		{
			if (!nums[i]) continue;
			sv.params.Change(i, cnum(dx*inertia[0], -dy*inertia[1]) * 0.005);
		}*/
	}
	else
	{
		int flags = 0;// [NSEvent modifierFlags];

		if (GetKeyState(VK_SHIFT) & 0x8000) flags |= SHIFT;
		//const bool alt = c & (1 << 13);
		//if (GetKeyState(VK_RMENU) & 0x8000) flags |= WIN;
		if (GetKeyState(VK_CONTROL) & 0x8000) flags |= CONTROL;

		//if (dx || dy) move(dx*inertia[0], dy*inertia[1], flags);
		//if (dz) move(0, dz*inertia[2], flags | SHIFT);
	}
}

void PlotView::OnKeyUp(UINT c, UINT rep, UINT flags)
{
	switch (c)
	{
		case      VK_LEFT: arrows.left  = false; inertia[0] = 0.0; break;
		case     VK_RIGHT: arrows.right = false; inertia[0] = 0.0; break;
		case        VK_UP: arrows.up    = false; inertia[1] = 0.0; break;
		case      VK_DOWN: arrows.down  = false; inertia[1] = 0.0; break;
		case  VK_OEM_PLUS:
		case       VK_ADD: arrows.plus  = false; inertia[2] = 0.0; break;
		case VK_OEM_MINUS:
		case  VK_SUBTRACT: arrows.minus = false; inertia[2] = 0.0; break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			int i = (c - '0' + 9) % 10;
			if (nums[i]) { --nums_on; nums[i] = false; }
			break;
		}
	}
	if (!arrows.all)
	{
		animating();
	}
}

void PlotView::OnKeyDown(UINT c, UINT rep, UINT flags)
{
	if ((flags & KF_REPEAT) && c != 'N') return;

	const bool ctrl  = GetKeyState(VK_CONTROL) & 0x8000;
	const bool shift = GetKeyState(VK_SHIFT) & 0x8000;
	const bool alt   = c & (1 << 13);
	const bool win   = GetKeyState(VK_RMENU) & 0x8000;

	bool done = true;
	switch (c)
	{
		case      VK_LEFT: arrows.left  = true; break;
		case     VK_RIGHT: arrows.right = true; break;
		case        VK_UP: arrows.up    = true; break;
		case      VK_DOWN: arrows.down  = true; break;
		case  VK_OEM_PLUS: 
		case       VK_ADD: arrows.plus  = true; break;
		case VK_OEM_MINUS:
		case  VK_SUBTRACT: arrows.minus = true; break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		{
			int i = (c - '0' + 9) % 10;
			if (!nums[i]) { ++nums_on; nums[i] = true; }
			break;
		}

		default: done = false;
	}

	if (done)
	{
		if (arrows.all) animating();
		return;
	}

	if (c >= 'A' && c <= 'Z' && !shift) c += 'a' - 'A';

	switch (c)
	{
		case ' ':
		{
			/*if (nums_on)
			{
				for (int i = 0; i < 10; ++i)
				{
					if (nums[i]) sv.params.ToggleAnimation(i);
				}
			}
			else*/
			{
				Graph &g = graph();
				g.animate(!g.animating());
				animating();
			}
			break;
		}
		case VK_BACK:
		case VK_CLEAR:
		case VK_DELETE:
		{
			Graph &g = graph();
			g.reset();
			Invalidate();
			break;
		}
		case 'n':
		{
			Invalidate();
			break;
		}
	}
}

void PlotView::OnChar(UINT c, UINT rep, UINT flags)
{
	// don't care, just don't beep on every key
}
