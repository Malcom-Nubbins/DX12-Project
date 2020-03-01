#pragma once
#include "../Globals/stdafx.h"
#include "../Globals/AppValues.h"

class AppRenderer_dx12
{
public:
	AppRenderer_dx12(const HWND& hWnd);
	~AppRenderer_dx12();

	void Cleanup();

	HRESULT Initialise();

	uint64_t Signal();
	void WaitForFenceValue();
	void Flush();

public:
	ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const { return m_CommandAllocators[m_CurrentBackBufferIndex]; }
	ComPtr<ID3D12Resource> GetBackBuffer() const { return m_BackBuffers[m_CurrentBackBufferIndex]; }
	ComPtr<ID3D12GraphicsCommandList> GetCommandList() const { return m_CommandList; }
	ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() const { return m_RTVDescriptorHeap; }
	ComPtr<ID3D12CommandQueue> GetCommandQueue() const { return m_CommandQueue; }
	ComPtr<IDXGISwapChain4> GetSwapChain() const { return m_SwapChain; }
	UINT GetCurrentBackBufferIndex() const { return m_CurrentBackBufferIndex; }
	UINT GetRTVDescriptorHeapSize() const { return m_RTVDescriptorSize; }

	void SetBackBufferIndex(UINT newBackBufferIndex) { m_CurrentBackBufferIndex = newBackBufferIndex; }
	void SetFrameFenceValue();

private:
	ComPtr<IDXGIAdapter4> GetAdapter();
	ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);
	ComPtr<ID3D12CommandQueue> CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type);
	ComPtr<IDXGISwapChain4> CreateSwapChain();
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

	void UpdateRenderTargetViews();

	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE);
	ComPtr<ID3D12GraphicsCommandList> CreateCommandList(D3D12_COMMAND_LIST_TYPE);
	ComPtr<ID3D12Fence> CreateFence();
	HANDLE CreateEventHandle();

private:

	HWND m_hWnd;

	static const UINT m_NumFrames = 3;
	bool m_IsInitialised = false;

	ComPtr<ID3D12Device2> m_Device;
	ComPtr<ID3D12CommandQueue> m_CommandQueue;
	ComPtr<IDXGISwapChain4> m_SwapChain;
	ComPtr<ID3D12Resource> m_BackBuffers[m_NumFrames];
	ComPtr<ID3D12GraphicsCommandList> m_CommandList;
	ComPtr<ID3D12CommandAllocator> m_CommandAllocators[m_NumFrames];
	ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
	ComPtr<ID3D12Fence> m_Fence;

	UINT m_RTVDescriptorSize;
	UINT m_CurrentBackBufferIndex;
	uint64_t m_FenceValue;
	uint64_t m_FrameFenceValues[m_NumFrames] = {};
	HANDLE m_FenceEvent;
};

