#include "AppWindow.h"
#include "AppEngineBase.h"
#include "../Application.h"
#include "../Globals/Helpers.h"
#include "CommandQueue.h"

AppWindow::AppWindow(HWND hWnd, const std::wstring& windowName, UINT clientWidth, UINT clientHeight, bool vsync)
	: m_hWnd(hWnd)
	, m_WindowName(windowName)
	, m_WindowWidth(clientWidth)
	, m_WindowHeight(clientHeight)
	, m_ScreenCentre()
	, m_WindowRect()
	, m_FrameCounter(0)
	, m_VSync(vsync)
	, m_Fullscreen(false)
{
	Application& app = Application::Get();

	m_IsTearingSupported = app.IsTearingSupported();

	m_SwapChain = CreateSwapChain();
	m_RTVDescriptorHeap = app.CreateDescriptorHeap(BufferCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_RTVDescriptorSize = app.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	UpdateRenderTargetViews();

	m_ScreenCentre.x = m_WindowWidth / 2;
	m_ScreenCentre.y = m_WindowHeight / 2;
}

AppWindow::~AppWindow()
{
}

void AppWindow::RegisterCallbacks(std::shared_ptr<AppEngineBase> pAppEngineBase)
{
	m_pEngineBase = pAppEngineBase;
}

void AppWindow::OnUpdate(UpdateEvent&)
{
	m_UpdateClock.Tick();

	if (auto pEngine = m_pEngineBase.lock())
	{
		m_FrameCounter++;

		UpdateEvent updateEvent(m_UpdateClock.GetDeltaSeconds(), m_UpdateClock.GetTotalSeconds());
		pEngine->OnUpdate(updateEvent);
	}
}

void AppWindow::OnRender(RenderEvent&)
{
	m_RenderClock.Tick();

	if (auto pEngine = m_pEngineBase.lock())
	{
		RenderEvent renderEvent(m_RenderClock.GetDeltaSeconds(), m_RenderClock.GetTotalSeconds());
		pEngine->OnRender(renderEvent);
	}
}

void AppWindow::OnKeyPressed(KeyEvent& e)
{
	if (auto pEngine = m_pEngineBase.lock())
	{
		pEngine->OnKeyPressed(e);
	}
}

void AppWindow::OnKeyReleased(KeyEvent& e)
{
	if (auto pEngine = m_pEngineBase.lock())
	{
		pEngine->OnKeyReleased(e);
	}
}

void AppWindow::OnMouseMoved(MouseMotionEvent& e)
{
	if (auto pEngine = m_pEngineBase.lock())
	{
		pEngine->OnMouseMoved(e);
	}
}

void AppWindow::OnMouseButtonDown(MouseButtonEvent& e)
{
	if (auto pEngine = m_pEngineBase.lock())
	{
		pEngine->OnMouseButtonDown(e);
	}
}

void AppWindow::OnMouseButtonUp(MouseButtonEvent& e)
{
	if (auto pEngine = m_pEngineBase.lock())
	{
		pEngine->OnMouseButtonUp(e);
	}
}

void AppWindow::OnMouseWheel(MouseWheelEvent& e)
{
	if (auto pEngine = m_pEngineBase.lock())
	{
		pEngine->OnMouseWheel(e);
	}
}

void AppWindow::OnResize(UINT width, UINT height)
{
	if (m_WindowWidth != width || m_WindowHeight != height)
	{
		m_WindowWidth = std::max(1, static_cast<int>(width));
		m_WindowHeight = std::max(1, static_cast<int>(height));

		Application::Get().Flush();

		for (int i = 0; i < BufferCount; ++i)
		{
			m_BackBuffers[i].Reset();
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(m_SwapChain->ResizeBuffers(BufferCount, m_WindowWidth, m_WindowHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

		UpdateRenderTargetViews();
	}

	if (auto pEngine = m_pEngineBase.lock())
	{
		pEngine->OnResize(width, height);
	}
}

void AppWindow::Cleanup()
{
	if (auto pEngine = m_pEngineBase.lock())
	{
		pEngine->OnWindowDestroyed();
	}

	for (int i = 0; i < BufferCount; ++i)
	{
		auto resource = m_BackBuffers[i].Get();
		m_BackBuffers[i].Reset();
	}

	if (m_hWnd)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
}


void AppWindow::Show()
{
	::ShowWindow(m_hWnd, SW_SHOW);
}

void AppWindow::Hide()
{
	::ShowWindow(m_hWnd, SW_HIDE);
}

void AppWindow::SetVSync(bool vsync)
{
	m_VSync = vsync;
}

void AppWindow::ToggleVSync()
{
	SetVSync(!m_VSync);
}

void AppWindow::SetFullscreen(bool fullscreen)
{
	if (m_Fullscreen != fullscreen)
	{
		m_Fullscreen = fullscreen;

		if (m_Fullscreen)
		{
			::GetWindowRect(m_hWnd, &m_WindowRect);

			UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			::SetWindowLongW(m_hWnd, GWL_STYLE, windowStyle);

			HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(hMonitor, &monitorInfo);

			::SetWindowPos(m_hWnd, HWND_TOP,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(m_hWnd, SW_MAXIMIZE);
		}
		else
		{
			::SetWindowLongW(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			::SetWindowPos(m_hWnd, HWND_TOP,
				m_WindowRect.left,
				m_WindowRect.top,
				m_WindowRect.right - m_WindowRect.left,
				m_WindowRect.bottom - m_WindowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(m_hWnd, SW_NORMAL);
		}
	}
}

void AppWindow::ToggleFullscreen()
{
	SetFullscreen(!m_Fullscreen);
}

UINT AppWindow::Present()
{
	UINT syncInterval = m_VSync ? 1 : 0;
	UINT presentFlags = m_IsTearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	return m_CurrentBackBufferIndex;
}

D3D12_CPU_DESCRIPTOR_HANDLE AppWindow::GetCurrentRenderTargetView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBufferIndex, m_RTVDescriptorSize);
}

ComPtr<ID3D12Resource> AppWindow::GetCurrentBackBuffer() const
{
	return m_BackBuffers[m_CurrentBackBufferIndex];
}

ComPtr<IDXGISwapChain4> AppWindow::CreateSwapChain()
{
	Application& app = Application::Get();
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = m_WindowWidth;
	swapChainDesc.Height = m_WindowHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = BufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = m_IsTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ID3D12CommandQueue* commandQueue = app.GetCommandQueue()->GetCommandQueue().Get();

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
		commandQueue,
		m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1));

	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

	m_CurrentBackBufferIndex = dxgiSwapChain4->GetCurrentBackBufferIndex();

	return dxgiSwapChain4;
}

void AppWindow::UpdateRenderTargetViews()
{
	auto device = Application::Get().GetDevice();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	for (int i = 0; i < BufferCount; ++i)
	{
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		m_BackBuffers[i] = backBuffer;
		rtvHandle.Offset(m_RTVDescriptorSize);
	}
}
