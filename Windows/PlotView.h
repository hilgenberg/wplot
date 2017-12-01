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
	void OnMButtonDown(UINT flags, CPoint p);
	void OnRButtonDown(UINT flags, CPoint p);
	void OnButtonDown (int i, CPoint p);
	void OnLButtonUp  (UINT flags, CPoint p);
	void OnMButtonUp  (UINT flags, CPoint p);
	void OnRButtonUp  (UINT flags, CPoint p);
	void OnButtonUp   (int i, CPoint p);

	void OnKeyDown   (UINT c, UINT rep, UINT flags);
	void OnKeyUp     (UINT c, UINT rep, UINT flags);
	void OnChar      (UINT c, UINT rep, UINT flags);
	UINT OnGetDlgCode();

private:
	void reshape();
	void handleArrows();
	bool animating();

	Graph      &graph() const;
	CClientDC  *dc;
	CRect       bounds;

	Timer    timer;
	double   tnf; // scheduled time for next frame
	double   last_frame; // time of last draw
	double   last_key; // time of last handleArrows
	GL_RM   *rm;
	bool     in_resize;

	union
	{
		struct
		{
			bool left : 1;
			bool right : 1;
			bool up : 1;
			bool down : 1;
			bool plus : 1;
			bool minus : 1;
		};
		char all;
	}
	arrows;

	double inertia[3]; // = {left/right, up/down, plus/minus}, all >= 0

	bool nums[10]; // keys to change params 0..9 (being '1',..'9','0') held down?
	short nums_on; // how many of them?

	typedef UINT KeySym;
	std::map<KeySym, double> ikeys; // pressed key -> inertia
	std::set<KeySym> keys; // pressed keys

	CPoint m0; // last mouse position (for OnMouseMove)
	union
	{
		struct
		{
			bool left : 1;
			bool middle : 1;
			bool right : 1;
		};
		char all;
	}
	mb; // currently held mouse buttons

	DECLARE_DYNCREATE(PlotView)
	DECLARE_MESSAGE_MAP()
};
