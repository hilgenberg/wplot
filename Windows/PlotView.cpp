#include "stdafx.h"
#include "PlotView.h"
#include "MainWindow.h"
#include "CPlotApp.h"
#include "../Graph.h"
#include "../Point.h"

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
	ON_WM_RBUTTONDOWN()

	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

PlotView::PlotView()
: dc(NULL)
, rm(NULL)
, timer(1.0/60.0)
{
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
	timer.stop();

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
	timer.run(anim);
	return anim;
}

void PlotView::OnPaint()
{
	CPaintDC dc(this);

	animating();

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
}

void PlotView::reshape()
{
	graph().viewport(bounds.Width(), bounds.Height());
}

//----------------------------------------------------------------------------------------------------------
//  Mouse
//----------------------------------------------------------------------------------------------------------

void PlotView::OnLButtonDown(UINT flags, CPoint p)
{
	Graph &g = graph();
	g.animate(!g.animating());
	animating();
}
void PlotView::OnRButtonDown(UINT flags, CPoint p)
{
	graph().reset();
	Invalidate();
}

//----------------------------------------------------------------------------------------------------------------------
//  keyboard
//----------------------------------------------------------------------------------------------------------------------

void PlotView::OnKeyDown(UINT c, UINT rep, UINT flags)
{
	if ((flags & KF_REPEAT) && c != 'N') return;

	const bool ctrl  = GetKeyState(VK_CONTROL) & 0x8000;
	const bool shift = GetKeyState(VK_SHIFT) & 0x8000;
	const bool alt   = c & (1 << 13);

	if (c >= 'A' && c <= 'Z' && !shift) c += 'a' - 'A';
	Graph &g = graph();

	switch (c)
	{
		case ' ':
			g.animate(!g.animating());
			animating();
			break;

		case VK_BACK:
		case VK_CLEAR:
		case VK_DELETE:
			g.reset();
			Invalidate();
			break;

		case 'n':
			Invalidate();
			break;

		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9': case '0':
		{
			int k = (c == '0' ? 10 : c - '0');
			ctrl ? g.zoom(k) : g.timezoom(k);
			if (!ctrl)
			{
				g.animate(true);
				animating();
			}
			Invalidate();
			break;
		}

		case 'q':
			GetParentFrame()->PostMessage(WM_CLOSE);
			break;

		case 'v':
			++Point::vis;
			Invalidate();
			break;
		case 'V':
			--Point::vis;
			Invalidate();
			break;

		case 'a': case 'b': case 'c': case 'd':
		case 'e': case 'f': case 'g': case 'h':
			Point::vis = c - 'a';
			Invalidate();
			break;
	}
}

void PlotView::OnChar(UINT c, UINT rep, UINT flags)
{
	// don't care, just don't beep on every key
}

void PlotView::OnKeyUp(UINT c, UINT rep, UINT flags)
{
}
