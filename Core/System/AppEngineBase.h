#pragma once
#include <string>
#include <memory>

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
	virtual bool UnloadContent() = 0;

	virtual void Cleanup();

protected:
	friend class AppWindow;

	virtual void OnUpdate();
	virtual void OnRender();

	virtual void OnKeyPressed();
	virtual void OnKeyReleased();
	virtual void OnMouseMoved();
	virtual void OnMouseButtonDown();
	virtual void OnMouseButtonUp();
	virtual void OnMouseWheel();

	virtual void OnResize(UINT width, UINT height);

	virtual void OnWindowDestroyed();

	std::shared_ptr<AppWindow> m_AppWindow;

private:
	std::wstring m_Name;
	UINT m_Width;
	UINT m_Height;
	bool m_vSync;
};

