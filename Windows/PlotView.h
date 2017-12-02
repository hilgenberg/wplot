#pragma once
#include "afxwin.h"
#include "../Graphs/GL_RM.h"
#include "../Utility/Timer.h"
class Graph;

class PlotView : public CWnd
{
public:
	PlotView();
	~PlotView();

	BOOL PreCreateWindow(CREATESTRUCT &cs);
	BOOL Create(const RECT &rect, CWnd *parent, UINT ID);
	int  OnCreate(LPCREATESTRUCT cs);
	void OnDestroy();

	BOOL OnEraseBkgnd(CDC *dc);
	void OnPaint();

	void OnSize(UINT type, int w, int h);

	void OnLButtonDown(UINT flags, CPoint p);
	void OnRButtonDown(UINT flags, CPoint p);

	void OnKeyDown   (UINT c, UINT rep, UINT flags);
	void OnKeyUp     (UINT c, UINT rep, UINT flags);
	void OnChar      (UINT c, UINT rep, UINT flags);
	UINT OnGetDlgCode();

private:
	void reshape();
	bool animating();

	Graph     &graph() const;
	CClientDC *dc;
	CRect      bounds;
	Timer      timer;
	GL_RM     *rm;

	DECLARE_DYNCREATE(PlotView)
	DECLARE_MESSAGE_MAP()
};
