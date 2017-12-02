#pragma once
#include "PlotView.h"
#include "../Graph.h"

class MainWindow : public CFrameWndEx
{
public:
	BOOL OnCreateClient(LPCREATESTRUCT cs, CCreateContext *ctx) override;
	BOOL OnEraseBkgnd(CDC *dc);
	BOOL OnCmdMsg(UINT ID, int code, void *extra, AFX_CMDHANDLERINFO *h);
	void OnSetFocus(CWnd *oldWnd);
	BOOL PreCreateWindow(CREATESTRUCT& cs);

	Graph    graph;
	PlotView plotView;

private:
	DECLARE_MESSAGE_MAP()
};
