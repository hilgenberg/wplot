#pragma once

#ifdef WIN10
#define DS0 const UINT dpi = ::GetDpiForWindow(GetSafeHwnd())
#else
#define DS0 \
	HDC tmp_hdc = ::GetDC(GetSafeHwnd());\
	const UINT dpi = ::GetDeviceCaps(tmp_hdc, LOGPIXELSX);\
	::ReleaseDC(NULL, tmp_hdc)
#endif

#define DS(x) MulDiv(x, dpi, 96)
