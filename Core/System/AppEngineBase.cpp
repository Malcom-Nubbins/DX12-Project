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

	m_AppWindow = Application::Get().CreateRenderWindow(m_Name, m_Width, m_Height);
	m_AppWindow->RegisterCallbacks(shared_from_this());
	m_AppWindow->Show();

	return true;
}

void AppEngineBase::Cleanup()
{
	Application::Get().DestroyWindow(m_AppWindow);
	m_AppWindow.reset();
}

void AppEngineBase::OnUpdate()
{
}

void AppEngineBase::OnRender()
{
}

void AppEngineBase::OnKeyPressed()
{
}

void AppEngineBase::OnKeyReleased()
{
}

void AppEngineBase::OnMouseMoved()
{
}

void AppEngineBase::OnMouseButtonDown()
{
}

void AppEngineBase::OnMouseButtonUp()
{
}

void AppEngineBase::OnMouseWheel()
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
