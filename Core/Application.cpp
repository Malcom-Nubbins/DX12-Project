#include "Application.h"

Application::Application() 
	: m_AppWindow(nullptr)
	, m_Renderer(nullptr)
{
}

Application::~Application()
{
}

void Application::Cleanup()
{
	m_Renderer->Cleanup();
}

HRESULT Application::Initialise(HINSTANCE hInst, int cmdShow)
{
	HRESULT hr;
	m_AppWindow = new AppWindow();
	hr = m_AppWindow->Initialise(hInst, cmdShow);
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Error creating window", L"Error", MB_OK);
		return hr;
	}

	m_Renderer = new AppRenderer_dx12(m_AppWindow->GetHWND());
	hr = m_Renderer->Initialise();
	if (FAILED(hr))
	{
		MessageBox(nullptr, L"Failed to initialise renderer", L"Error", MB_OK);
		return hr;
	}

	return S_OK;
}

LRESULT Application::HandleInput(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

void Application::Update(float deltaTime)
{
}

void Application::Draw()
{
	auto commandAllocator = m_Renderer->GetCommandAllocator();
	auto backBuffer = m_Renderer->GetBackBuffer();

	commandAllocator->Reset();
	auto commandList = m_Renderer->GetCommandList();
	commandList->Reset(commandAllocator.Get(), nullptr);

	// Clear render target
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandList->ResourceBarrier(1, &barrier);

		FLOAT clearColour[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_Renderer->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
			m_Renderer->GetCurrentBackBufferIndex(), m_Renderer->GetRTVDescriptorHeapSize());

		commandList->ClearRenderTargetView(rtv, clearColour, 0, nullptr);
	}

	// Present
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		commandList->ResourceBarrier(1, &barrier);

		HRESULT hr = commandList->Close();
		if (FAILED(hr))
		{
			throw new std::exception("Failed to close command list");
		}

		ID3D12CommandList* const commandLists[] = {
			commandList.Get() 
		};

		m_Renderer->GetCommandQueue()->ExecuteCommandLists(_countof(commandLists), commandLists);
		m_Renderer->SetFrameFenceValue();

		hr = m_Renderer->GetSwapChain()->Present(1, 0);
		if (FAILED(hr))
		{
			throw new std::exception("Failed to present");
		}

		m_Renderer->SetBackBufferIndex(m_Renderer->GetSwapChain()->GetCurrentBackBufferIndex());

		m_Renderer->WaitForFenceValue();
	}
}
