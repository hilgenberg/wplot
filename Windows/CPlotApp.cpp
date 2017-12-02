#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "CPlotApp.h"
#include "MainWindow.h"
#include "PlotView.h"
#include "res/resource.h"

#include "../Graph.h"
#include "PlotView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CPlotApp theApp;

BOOL CPlotApp::InitInstance()
{
	CWinAppEx::InitInstance();
	SetRegistryKey(_T("WPlot 1.0"));

	MainWindow *w = new MainWindow;
	m_pMainWnd = w;
	w->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL);
	w->ShowWindow(SW_SHOW);
	w->UpdateWindow();
	w->DragAcceptFiles(false);

	return TRUE;
}
