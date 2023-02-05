#pragma once

#include "../Globals/stdafx.h"
#include "../Globals/AppValues.h"
#include "../Globals/Events.h"
#include "Timer.h"

#include <string>
#include <memory>

class AppEngineBase;

class AppWindow
{
public:
	static const UINT BufferCount = 3;
	void Cleanup();

	HWND GetHWND() const { return m_hWnd; }

	UINT GetWindowWidth() const { return m_WindowWidth; }
	UINT GetWindowHeight() const { return m_WindowHeight; }

	POINT GetScreenCentre() const { return m_ScreenCentre; }

	const std::wstring& GetWindowName() const { return m_WindowName; }

	void Show();
	void Hide();

	bool IsVSync() const { return m_VSync; }
	void SetVSync(bool vsync);
	void ToggleVSync();

	bool IsFullscreen() const { return m_Fullscreen; }
	void SetFullscreen(bool fullscreen);
	void ToggleFullscreen();

	UINT GetCurrentBackBufferIndex() const { return m_CurrentBackBufferIndex; }

	UINT Present();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;
	ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;

protected:
	friend LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	friend class Application;
	friend class AppEngineBase;

	AppWindow() = delete;
	AppWindow(HWND hWnd, const std::wstring& windowName, UINT clientWidth, UINT clientHeight, bool vsync);
	virtual ~AppWindow();

	void RegisterCallbacks(std::shared_ptr<AppEngineBase> pAppEngineBase);

	virtual void OnUpdate(UpdateEvent& e);
	virtual void OnRender(RenderEvent& e);

	virtual void OnKeyPressed(KeyEvent& e);
	virtual void OnKeyReleased(KeyEvent& e);
	virtual void OnMouseMoved(MouseMotionEvent& e);
	virtual void OnMouseButtonDown(MouseButtonEvent& e);
	virtual void OnMouseButtonUp(MouseButtonEvent& e);
	virtual void OnMouseWheel(MouseWheelEvent& e);

	virtual void OnResize(UINT width, UINT height);

	ComPtr<IDXGISwapChain4> CreateSwapChain();

	void UpdateRenderTargetViews();

private:
	AppWindow(const AppWindow& copy) = delete;
	AppWindow& operator=(const AppWindow& other) = delete;

	HWND m_hWnd;

	std::wstring m_WindowName;

	UINT m_WindowWidth;
	UINT m_WindowHeight;
	bool m_VSync;
	bool m_Fullscreen;

	POINT m_ScreenCentre;

	RECT m_WindowRect;
	bool m_IsTearingSupported;

	std::weak_ptr<AppEngineBase> m_pEngineBase;

	ComPtr<IDXGISwapChain4> m_SwapChain;
	ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
	ComPtr<ID3D12Resource> m_BackBuffers[BufferCount];

	UINT m_RTVDescriptorSize;
	UINT m_CurrentBackBufferIndex;

	Timer m_UpdateClock;
	Timer m_RenderClock;
	uint64_t m_FrameCounter;
};

