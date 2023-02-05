#include "AppRenderer_dx12.h"

AppRenderer_dx12::AppRenderer_dx12(const HWND& hWnd) 
	: m_hWnd(hWnd)
	, m_CurrentBackBufferIndex(0)
	, m_RTVDescriptorSize(0)
	, m_FenceValue(0)
	, m_FenceEvent()
{
}

AppRenderer_dx12::~AppRenderer_dx12()
{
}

void AppRenderer_dx12::Cleanup()
{
	Flush();
	CloseHandle(m_FenceEvent);

	m_CommandQueue->Release();
	m_CommandList->Release();

	for (int i = 0; i < m_NumFrames; ++i)
	{
		m_BackBuffers[i]->Release();
	}

	for (int i = 0; i < m_NumFrames; ++i)
	{
		m_CommandAllocators[i]->Release();
	}

	m_RTVDescriptorHeap->Release();
	m_Fence->Release();
	m_SwapChain->Release();
	m_Device->Release();
}

HRESULT AppRenderer_dx12::Initialise()
{
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
	{
		debugInterface->EnableDebugLayer();
	}
#endif

	ComPtr<IDXGIAdapter4> adapter = GetAdapter();

	m_Device = CreateDevice(adapter);
	m_CommandQueue = CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_SwapChain = CreateSwapChain();

	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	m_RTVDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_NumFrames);
	m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	UpdateRenderTargetViews();

	for (int i = 0; i < m_NumFrames; ++i)
	{
		m_CommandAllocators[i] = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	//m_CommandList = CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	//m_Fence = CreateFence();
	//m_FenceEvent = CreateEventHandle();

	m_IsInitialised = true;

	return S_OK;
}

uint64_t AppRenderer_dx12::Signal()
{
	uint64_t fenceValueForSignal = ++m_FenceValue;
	HRESULT hr = m_CommandQueue->Signal(m_Fence.Get(), fenceValueForSignal);
	if (FAILED(hr))
	{
		throw new std::exception("Failed to signal");
	}

	return fenceValueForSignal;
}

void AppRenderer_dx12::WaitForFenceValue()
{
	std::chrono::milliseconds duration = std::chrono::milliseconds::max();
	if (m_Fence->GetCompletedValue() < m_FenceValue)
	{
		HRESULT hr = m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent);
		if (FAILED(hr))
		{
			throw new std::exception();
		}
		WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(duration.count()));
	}
}

void AppRenderer_dx12::Flush()
{
	uint64_t fenceValueForSignal = Signal();
	WaitForFenceValue();
}

void AppRenderer_dx12::SetFrameFenceValue()
{
	m_FrameFenceValues[m_CurrentBackBufferIndex] = Signal();
}

ComPtr<IDXGIAdapter4> AppRenderer_dx12::GetAdapter()
{
	HRESULT hr;
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr))
	{
		throw new std::exception("Could not create DXGI Factory");
	}

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
			hr = dxgiAdapter1.As(&dxgiAdapter4);
			if (SUCCEEDED(hr))
			{
				continue;
			}
			else
			{
				throw new std::exception();
			}
		}
	}

	return dxgiAdapter4;
}

ComPtr<ID3D12Device2> AppRenderer_dx12::CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
	HRESULT hr;
	ComPtr<ID3D12Device2> d3d12Device2;

	hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2));
	if (FAILED(hr))
	{
		throw new std::exception("Failed to create device");
	}

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

		hr = pInfoQueue->PushStorageFilter(&NewFilter);
		if (FAILED(hr))
		{
			throw new std::exception();
		}
	}
#endif

	return d3d12Device2;
}

ComPtr<ID3D12CommandQueue> AppRenderer_dx12::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

	return d3d12CommandQueue;
}

ComPtr<IDXGISwapChain4> AppRenderer_dx12::CreateSwapChain()
{
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	HRESULT hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4));
	if (FAILED(hr))
	{
		throw new std::exception();
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = WINDOW_WIDTH;
	swapChainDesc.Height = WINDOW_HEIGHT;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = m_NumFrames;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	hr = dxgiFactory4->CreateSwapChainForHwnd(
		m_CommandQueue.Get(),
		m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1);

	if (FAILED(hr))
	{
		throw new std::exception("Failed to create swapchain");
	}

	hr = dxgiFactory4->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr))
	{
		throw new std::exception();
	}

	hr = swapChain1.As(&dxgiSwapChain4);
	if (FAILED(hr))
	{
		throw new std::exception();
	}

	return dxgiSwapChain4;
}

ComPtr<ID3D12DescriptorHeap> AppRenderer_dx12::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;

	HRESULT hr = m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap));
	if (FAILED(hr))
	{
		throw new std::exception("Failed to create descriptor heap");
	}

	return descriptorHeap;
}

void AppRenderer_dx12::UpdateRenderTargetViews()
{
	UINT rtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	HRESULT hr;
	for (int i = 0; i < m_NumFrames; ++i)
	{
		ComPtr<ID3D12Resource> backBuffer;
		hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
		if (FAILED(hr))
		{
			throw new std::exception();
		}

		m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
		m_BackBuffers[i] = backBuffer;
		rtvHandle.Offset(rtvDescriptorSize);
	}
}

ComPtr<ID3D12CommandAllocator> AppRenderer_dx12::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;

	HRESULT hr = m_Device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator));
	if (FAILED(hr))
	{
		throw new std::exception("Failed to create command allocator");
	}

	return commandAllocator;
}
