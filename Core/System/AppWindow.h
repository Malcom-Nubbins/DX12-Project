#pragma once

#include "../Globals/stdafx.h"
#include "../Globals/AppValues.h"

class AppWindow
{
public:
	AppWindow();
	~AppWindow();

	void Cleanup();

	HRESULT Initialise(HINSTANCE hInst, int cmdShow);

	LRESULT CALLBACK RealWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND GetHWND() const { return m_hWnd; }

	UINT GetWindowWidth() const { return m_WindowWidth; }
	UINT GetWindowHeight() const { return m_WindowHeight; }

	POINT GetScreenCentre() const { return m_ScreenCentre; }

private:
	HWND m_hWnd;
	HINSTANCE m_hInstance;

	UINT m_WindowWidth;
	UINT m_WindowHeight;

	POINT m_ScreenCentre;

	RECT m_WindowRect;
};

