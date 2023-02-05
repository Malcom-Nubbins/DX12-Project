#pragma once
#include <string>
#include <memory>

#include "../Globals/Events.h"

class AppWindow;

class AppEngineBase : public std::enable_shared_from_this<AppEngineBase>
{
public:
	AppEngineBase(const std::wstring& name, UINT width, UINT height, bool vsync);
	virtual ~AppEngineBase();

	UINT GetClientWidth() const { return m_Width; }
	UINT GetClientHeight() const { return m_Height; }

	virtual bool Initialise();

	virtual bool LoadContent() = 0;
	virtual void UnloadContent() = 0;

	virtual void Cleanup();

protected:
	friend class AppWindow;

	virtual void OnUpdate(UpdateEvent& e);
	virtual void OnRender(RenderEvent& e);

	virtual void OnKeyPressed(KeyEvent& e);
	virtual void OnKeyReleased(KeyEvent& e);
	virtual void OnMouseMoved(MouseMotionEvent& e);
	virtual void OnMouseButtonDown(MouseButtonEvent& e);
	virtual void OnMouseButtonUp(MouseButtonEvent& e);
	virtual void OnMouseWheel(MouseWheelEvent& e);

	virtual void OnResize(UINT width, UINT height);

	virtual void OnWindowDestroyed();

	std::shared_ptr<AppWindow> m_AppWindow;

private:
	std::wstring m_Name;
	UINT m_Width;
	UINT m_Height;
	bool m_vSync;
};

