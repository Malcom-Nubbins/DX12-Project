#include "../Globals/stdafx.h"
#include "AppEngineBase.h"
#include "../Application.h"
#include "AppWindow.h"

AppEngineBase::AppEngineBase(const std::wstring& name, UINT width, UINT height, bool vsync)
	: m_Name(name)
	, m_Width(width)
	, m_Height(height)
	, m_vSync(vsync)
{
}

AppEngineBase::~AppEngineBase()
{
}

bool AppEngineBase::Initialise()
{
	if (!DirectX::XMVerifyCPUSupport())
	{
		MessageBoxA(nullptr, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
		return false;
	}

	m_AppWindow = Application::Get().CreateRenderWindow(m_Name, m_Width, m_Height, m_vSync);
	m_AppWindow->RegisterCallbacks(shared_from_this());
	m_AppWindow->Show();

	return true;
}

void AppEngineBase::Cleanup()
{
	Application::Get().DestroyWindow(m_AppWindow);
	m_AppWindow.reset();
}

void AppEngineBase::OnUpdate(UpdateEvent& e)
{
}

void AppEngineBase::OnRender(RenderEvent& e)
{
}

void AppEngineBase::OnKeyPressed(KeyEvent& e)
{
}

void AppEngineBase::OnKeyReleased(KeyEvent& e)
{
}

void AppEngineBase::OnMouseMoved(MouseMotionEvent& e)
{
}

void AppEngineBase::OnMouseButtonDown(MouseButtonEvent& e)
{
}

void AppEngineBase::OnMouseButtonUp(MouseButtonEvent& e)
{
}

void AppEngineBase::OnMouseWheel(MouseWheelEvent& e)
{
}

void AppEngineBase::OnResize(UINT width, UINT height)
{
	m_Width = width;
	m_Height = height;
}

void AppEngineBase::OnWindowDestroyed()
{
	UnloadContent();
}
