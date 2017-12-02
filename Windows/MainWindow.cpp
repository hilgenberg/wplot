#include "stdafx.h"
#include "CPlotApp.h"
#include "MainWindow.h"
#include "../Graph.h"
#include "PlotView.h"
#include "res/resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(MainWindow, CFrameWndEx)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

BOOL MainWindow::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.hMenu = NULL;
	return CFrameWndEx::PreCreateWindow(cs);
}

BOOL MainWindow::OnCreateClient(LPCREATESTRUCT cs, CCreateContext *ctx)
{
	if (!CFrameWnd::OnCreateClient(cs, ctx)) return false;
	CRect b; GetClientRect(b);
	plotView.Create(b, this, AFX_IDW_PANE_FIRST);
	plotView.ShowWindow(SW_SHOW);
	return TRUE;
}

BOOL MainWindow::OnEraseBkgnd(CDC *dc)
{
	return false;
}

BOOL MainWindow::OnCmdMsg(UINT ID, int code, void *extra, AFX_CMDHANDLERINFO *h)
{
	return plotView.OnCmdMsg(ID, code, extra, h) ||
		CFrameWndEx::OnCmdMsg(ID, code, extra, h);
}

void MainWindow::OnSetFocus(CWnd*)
{
	plotView.SetFocus();
}

