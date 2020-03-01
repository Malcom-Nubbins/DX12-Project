#include "Globals/stdafx.h"
#include "Application.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	MSG msg = { 0 };

	Application* m_App = new Application();
	if (FAILED(m_App->Initialise(hInstance, nCmdShow)))
	{
		return -1;
	}

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			m_App->Update(0);
			m_App->Draw();
		}
	}

	m_App->Cleanup();
	delete m_App;
	m_App = nullptr;

	return (int)msg.wParam;
}