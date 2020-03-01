#include "AppWindow.h"

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	AppWindow* me = reinterpret_cast<AppWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (me)
	{
		return me->RealWndProc(hWnd, msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK AppWindow::RealWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

AppWindow::AppWindow() 
	: m_hWnd(nullptr)
	, m_hInstance(nullptr)
	, m_WindowWidth(0)
	, m_WindowHeight(0)
	, m_ScreenCentre()
	, m_WindowRect()
{
}

AppWindow::~AppWindow()
{
}

void AppWindow::Cleanup()
{
	m_hInstance = nullptr;
	m_hWnd = nullptr;
}

HRESULT AppWindow::Initialise(HINSTANCE hInst, int cmdShow)
{
	m_hInstance = hInst;

	m_WindowWidth = WINDOW_WIDTH;
	m_WindowHeight = WINDOW_HEIGHT;

	m_ScreenCentre.x = m_WindowWidth / 2;
	m_ScreenCentre.y = m_WindowHeight / 2;

	float posX = static_cast<float>((GetSystemMetrics(SM_CXSCREEN) - m_WindowWidth) / 2);
	float posY = static_cast<float>((GetSystemMetrics(SM_CYSCREEN) - m_WindowHeight) / 2);

	WNDCLASSEXW windowClass = {};

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = m_hInstance;
	windowClass.hIcon = (HICON)LoadImage(m_hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
	windowClass.hCursor = ::LoadCursorW(m_hInstance, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = L"AppWindow";
	windowClass.hIconSm = (HICON)LoadImage(m_hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);

	if (!RegisterClassEx(&windowClass))
	{
		return E_FAIL;
	}

	m_WindowRect = { 0, 0, static_cast<LONG>(m_WindowWidth), static_cast<LONG>(m_WindowHeight) };
	AdjustWindowRect(&m_WindowRect, WS_OVERLAPPEDWINDOW, false);

	m_hWnd = CreateWindowEx(NULL, L"AppWindow", L"DirectX 12 Project", WS_OVERLAPPEDWINDOW, posX, posY, m_WindowWidth, m_WindowHeight, nullptr, nullptr, m_hInstance, nullptr);

	if (!m_hWnd)
	{
		return E_FAIL;
	}

	SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	ShowWindow(m_hWnd, cmdShow);

	return S_OK;
}
