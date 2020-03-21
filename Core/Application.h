#pragma once
#include "Globals/stdafx.h"

#include "System/AppRenderer_dx12.h"


class AppEngineBase;
class AppWindow;
class CommandQueue;

class Application
{
public:

	static void Create(HINSTANCE hInst);
	static void Destroy();

	static Application& Get();

	std::shared_ptr<AppWindow> CreateRenderWindow(const std::wstring& windowName, UINT clientWidth, UINT clientHeight);

	void DestroyWindow(const std::wstring& windowName);
	void DestroyWindow(std::shared_ptr<AppWindow> window);

	std::shared_ptr<AppWindow> GetWindowByName(const std::wstring& windowName);

	int Run(std::shared_ptr<AppEngineBase> pEngineBase);
	void Quit(int exitCode = 0);

	ComPtr<ID3D12Device2> GetDevice() const;
	std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;

	void Flush();

	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type);
	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

protected:
	Application(HINSTANCE hInst);
	virtual ~Application();

	void Initialise();

	ComPtr<IDXGIAdapter4> GetAdapter();
	ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);

private:
	Application(const Application& copy) = delete;
	Application& operator=(const Application& other) = delete;

	HINSTANCE m_hInstance;
	ComPtr<ID3D12Device2> m_Device;

	std::shared_ptr<CommandQueue> m_DirectCommandQueue;
	std::shared_ptr<CommandQueue> m_ComputeCommandQueue;
	std::shared_ptr<CommandQueue> m_CopyCommandQueue;
};

