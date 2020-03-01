#pragma once
#include "Globals/stdafx.h"

#include "System/AppWindow.h"
#include "System/AppRenderer_dx12.h"

class Application
{
public:
	Application();
	~Application();

	void Cleanup();

	HRESULT Initialise(HINSTANCE hInst, int cmdShow);
	LRESULT HandleInput(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void Update(float deltaTime);
	void Draw();

private:
	AppWindow* m_AppWindow;
	AppRenderer_dx12* m_Renderer;
};

