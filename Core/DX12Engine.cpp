#include "DX12Engine.h"
#include "Application.h"
#include "System/CommandQueue.h"

DX12Engine::DX12Engine(const std::wstring& name, UINT width, UINT height, bool vsync)
	: super(name, width, height, vsync)
{
}

bool DX12Engine::LoadContent()
{
	return true;
}

bool DX12Engine::UnloadContent()
{
	return false;
}

void DX12Engine::OnUpdate()
{
}

void DX12Engine::OnRender()
{
	super::OnRender();

	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();

	UINT currentBackBufferIndex = m_AppWindow->GetCurrentBackBufferIndex();
	auto backBuffer = m_AppWindow->GetCurrentBackBuffer();
	auto rtv = m_AppWindow->GetCurrentRenderTargetView();

	// Clear render targets
	{
		TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		FLOAT clearColour[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		ClearRTV(commandList, rtv, clearColour);
	}

	commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

	// Present
	{
		TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		
		m_FenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

		currentBackBufferIndex = m_AppWindow->Present();
		commandQueue->WaitForFenceValue(m_FenceValues[currentBackBufferIndex]);
	}
}

void DX12Engine::OnKeyPressed()
{
}

void DX12Engine::OnMouseWheel()
{
}

void DX12Engine::OnResize(UINT width, UINT height)
{
	if (width != GetClientWidth() || height != GetClientHeight())
	{
		super::OnResize(width, height);
	}
}

void DX12Engine::TransitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
	commandList->ResourceBarrier(1, &barrier);
}

void DX12Engine::ClearRTV(ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColour)
{
	commandList->ClearRenderTargetView(rtv, clearColour, 0, nullptr);
}
