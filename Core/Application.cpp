#include "Application.h"
#include "Globals/Helpers.h"

#include "System/AppEngineBase.h"
#include "System/CommandQueue.h"
#include "System/AppWindow.h"

#include <map>

constexpr wchar_t WINDOW_CLASS_NAME[] = L"DX12RenderWindowClass";

using WindowPtr = std::shared_ptr<AppWindow>;
using WindowMap = std::map<HWND, WindowPtr>;
using WindowNameMap = std::map<std::wstring, WindowPtr>;

static Application* g_App = nullptr;
static WindowMap g_Windows;
static WindowNameMap g_WindowByName;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

uint64_t Application::m_FrameCount = 0;

struct MakeWindow : public AppWindow
{
	MakeWindow(HWND hWnd, const std::wstring& windowName, UINT clientWidth, UINT clientHeight, bool vsync)
		: AppWindow(hWnd, windowName, clientWidth, clientHeight, vsync)
	{}
};

void Application::Flush()
{
	m_DirectCommandQueue->Flush();
	m_ComputeCommandQueue->Flush();
	m_CopyCommandQueue->Flush();
}

ComPtr<ID3D12DescriptorHeap> Application::CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

UINT Application::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	return m_Device->GetDescriptorHandleIncrementSize(type);
}

Application::Application(HINSTANCE hInst)
	: m_hInstance(hInst)
	, m_TearingSupported(false)
{
}

Application::~Application()
{
	Flush();
}

void Application::Initialise()
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif

	WNDCLASSEXW windowClass = { 0 };

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &WndProc;
	windowClass.hInstance = m_hInstance;
	windowClass.hIcon = (HICON)LoadImage(m_hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
	windowClass.hCursor = ::LoadCursorW(m_hInstance, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = nullptr;
	windowClass.lpszClassName = WINDOW_CLASS_NAME;
	windowClass.hIconSm = (HICON)LoadImage(m_hInstance, MAKEINTRESOURCE(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);

	if (!RegisterClassEx(&windowClass))
	{
		MessageBoxA(nullptr, "Unable to register the window class", "Error", MB_OK | MB_ICONERROR);
	}

	auto dxgiAdapter = GetAdapter();
	if (dxgiAdapter)
	{
		m_Device = CreateDevice(dxgiAdapter);
	}

	m_DirectCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_ComputeCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_CopyCommandQueue = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY);

	m_TearingSupported = CheckTearingSupport();
}

ComPtr<IDXGIAdapter4> Application::GetAdapter()
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	SIZE_T maxDedicatedVideoMemory = 0;
	for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
		dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

		if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
			SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
			dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
			ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
		}
	}

	return dxgiAdapter4;
}

ComPtr<ID3D12Device2> Application::CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
	ComPtr<ID3D12Device2> d3d12Device2;

	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
	
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		NewFilter.DenyList.NumSeverities = _countof(severities);
		NewFilter.DenyList.pSeverityList = severities;
		NewFilter.DenyList.NumIDs = _countof(denyIds);
		NewFilter.DenyList.pIDList = denyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif

	return d3d12Device2;
}

bool Application::CheckTearingSupport()
{
	BOOL allowTearing = FALSE;

	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing));
		}
	}

	return allowTearing == TRUE;
}


void Application::Create(HINSTANCE hInst)
{
	if (!g_App)
	{
		g_App = new Application(hInst);
		g_App->Initialise();
	}
}

Application& Application::Get()
{
	return *g_App;
}

void Application::Destroy()
{
	if (g_App)
	{
		assert(g_Windows.empty() && g_WindowByName.empty() && "All windows must be destroyed first");

		delete g_App;
		g_App = nullptr;
	}
}

std::shared_ptr<AppWindow> Application::CreateRenderWindow(const std::wstring& windowName, UINT clientWidth, UINT clientHeight, bool vsync)
{
	WindowNameMap::iterator windowIter = g_WindowByName.find(windowName);
	if (windowIter != g_WindowByName.end())
	{
		return windowIter->second;
	}

	RECT windowRect = { 0, 0, clientWidth, clientHeight };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hwnd = CreateWindowW(WINDOW_CLASS_NAME, windowName.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr, nullptr, m_hInstance, nullptr);

	if (!hwnd)
	{
		MessageBoxA(nullptr, "Could not create the window", "Error", MB_OK | MB_ICONERROR);
		return nullptr;
	}

	WindowPtr pWindow = std::make_shared<MakeWindow>(hwnd, windowName, clientWidth, clientHeight, vsync);

	g_Windows.insert(WindowMap::value_type(hwnd, pWindow));
	g_WindowByName.insert(WindowNameMap::value_type(windowName, pWindow));

	return pWindow;
}

void Application::DestroyWindow(const std::wstring& windowName)
{
	WindowPtr window = GetWindowByName(windowName);
	if (window)
	{
		DestroyWindow(window);
	}
}

void Application::DestroyWindow(std::shared_ptr<AppWindow> window)
{
	if (window)
	{
		window->Cleanup();
	}
}

std::shared_ptr<AppWindow> Application::GetWindowByName(const std::wstring& windowName)
{
	std::shared_ptr<AppWindow> window;
	WindowNameMap::iterator iter = g_WindowByName.find(windowName);
	if (iter != g_WindowByName.end())
	{
		window = iter->second;
	}

	return window;
}

int Application::Run(std::shared_ptr<AppEngineBase> pEngineBase)
{
	if (!pEngineBase->Initialise()) return 1;
	if (!pEngineBase->LoadContent()) return 2;

	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Flush();

	pEngineBase->UnloadContent();
	pEngineBase->Cleanup();

	return static_cast<int>(msg.wParam);
}

void Application::Quit(int exitCode)
{
	PostQuitMessage(exitCode);
}

ComPtr<ID3D12Device2> Application::GetDevice() const
{
	return m_Device;
}

std::shared_ptr<CommandQueue> Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
	std::shared_ptr<CommandQueue> commandQueue;
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		commandQueue = m_DirectCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		commandQueue = m_ComputeCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		commandQueue = m_CopyCommandQueue;
		break;
	}

	return commandQueue;
}

static void RemoveWindow(HWND hwnd)
{
	WindowMap::iterator iter = g_Windows.find(hwnd);
	if (iter != g_Windows.end())
	{
		WindowPtr window = iter->second;
		g_WindowByName.erase(window->GetWindowName());
		g_Windows.erase(iter);
	}
}

MouseButtonEvent::MouseButton DecodeMouseButton(UINT messageID)
{
	MouseButtonEvent::MouseButton mouseButton = MouseButtonEvent::None;
	switch (messageID)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEvent::Left;
	}
	break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEvent::Right;
	}
	break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEvent::Middle;
	}
	break;
	}

	return mouseButton;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WindowPtr pWindow;
	{
		WindowMap::iterator iter = g_Windows.find(hwnd);
		if (iter != g_Windows.end())
		{
			pWindow = iter->second;
		}
	}

	if (pWindow)
	{
		switch (message)
		{
		case WM_PAINT:
		{
			UpdateEvent updateEvent(0.0, 0.0);
			pWindow->OnUpdate(updateEvent);

			RenderEvent renderEvent(0.0, 0.0);
			pWindow->OnRender(renderEvent);
		}
		break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			MSG charMsg;
			unsigned int c = 0;

			if (PeekMessage(&charMsg, hwnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
			{
				GetMessage(&charMsg, hwnd, 0, 0);
				c = static_cast<unsigned int>(charMsg.wParam);
			}

			bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

			KeyCode::Key key = (KeyCode::Key)wParam;
			unsigned int scanCode = (lParam & 0x00FF0000) >> 16;
			KeyEvent keyEvent(key, c, KeyEvent::Pressed, ctrl, shift, alt);
			pWindow->OnKeyPressed(keyEvent);
		}
		break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			KeyCode::Key key = (KeyCode::Key)wParam;
			unsigned int c = 0;
			unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

			unsigned char keyboardState[256];
			BOOL KeyboardStateRet = GetKeyboardState(keyboardState);
			wchar_t translatedCharacters[4];

			if (int result = ToUnicodeEx(static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
			{
				c = translatedCharacters[0];
			}

			KeyEvent keyEvent(key, c, KeyEvent::Released, ctrl, shift, alt);
			pWindow->OnKeyPressed(keyEvent);
		}
		break;

		case WM_SYSCHAR:
			break;

		case WM_MOUSEMOVE:
		{
			bool lmButton = (wParam & MK_LBUTTON) != 0;
			bool rmButton = (wParam & MK_RBUTTON) != 0;
			bool mmButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool ctrl = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseMotionEvent motionEvent(lmButton, mmButton, rmButton, ctrl, shift, x, y);
			pWindow->OnMouseMoved(motionEvent);
		}
		break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			bool lmButton = (wParam & MK_LBUTTON) != 0;
			bool rmButton = (wParam & MK_RBUTTON) != 0;
			bool mmButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool ctrl = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseButtonEvent mouseButtonEvent(DecodeMouseButton(message), MouseButtonEvent::Pressed, lmButton, mmButton, rmButton, ctrl, shift, x, y);
			pWindow->OnMouseButtonDown(mouseButtonEvent);
		}
		break;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			MouseButtonEvent mouseButtonEvent(DecodeMouseButton(message), MouseButtonEvent::Released, lButton, mButton, rButton, control, shift, x, y);
			pWindow->OnMouseButtonUp(mouseButtonEvent);
		}
		break;
		case WM_MOUSEWHEEL:
		{
			// The distance the mouse wheel is rotated.
			// A positive value indicates the wheel was rotated to the right.
			// A negative value indicates the wheel was rotated to the left.
			float zDelta = ((int)(short)HIWORD(wParam)) / (float)WHEEL_DELTA;
			short keyStates = (short)LOWORD(wParam);

			bool lButton = (keyStates & MK_LBUTTON) != 0;
			bool rButton = (keyStates & MK_RBUTTON) != 0;
			bool mButton = (keyStates & MK_MBUTTON) != 0;
			bool shift = (keyStates & MK_SHIFT) != 0;
			bool control = (keyStates & MK_CONTROL) != 0;

			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));

			// Convert the screen coordinates to client coordinates.
			POINT clientToScreenPoint;
			clientToScreenPoint.x = x;
			clientToScreenPoint.y = y;
			ScreenToClient(hwnd, &clientToScreenPoint);

			MouseWheelEvent mouseWheelEvent(zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
			pWindow->OnMouseWheel(mouseWheelEvent);
		}
		break;
		case WM_SIZE:
		{
			UINT width = static_cast<UINT>(LOWORD(lParam));
			UINT height = static_cast<UINT>(HIWORD(lParam));
			pWindow->OnResize(width, height);
		}
		break;

		case WM_DESTROY:
		{
			RemoveWindow(hwnd);
			if (g_Windows.empty())
			{
				PostQuitMessage(0);
			}
		}
		break;

		default:
			return DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}
	else
	{
		return DefWindowProcW(hwnd, message, wParam, lParam);
	}

	return 0;
}
