#pragma once
#include "Globals/stdafx.h"
#include "System/AppEngineBase.h"
#include "System/AppWindow.h"


class DX12Engine : public AppEngineBase
{
public:
	using super = AppEngineBase;

	DX12Engine(const std::wstring& name, UINT width, UINT height, bool vsync = false);
	
	virtual bool LoadContent() override;
	virtual void UnloadContent() override;

protected:
	virtual void OnUpdate(UpdateEvent& e) override;
	virtual void OnRender(RenderEvent& e) override;

	virtual void OnKeyPressed(KeyEvent& e) override;
	virtual void OnMouseWheel(MouseWheelEvent& e) override;
	virtual void OnResize(UINT width, UINT height) override;

private:
	void TransitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
		ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	void ClearRTV(ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColour);

	void ClearDepth(ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

	void UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void ResizeDepthBuffer(UINT width, UINT height);

	uint64_t m_FenceValues[AppWindow::BufferCount] = {};

	ComPtr<ID3D12Resource> m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	ComPtr<ID3D12Resource> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	ComPtr<ID3D12Resource> m_DepthBuffer;
	ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

	ComPtr<ID3D12RootSignature> m_RootSignature;

	ComPtr<ID3D12PipelineState> m_PipelineState;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	float m_FOV;

	DirectX::XMMATRIX m_WorldMatrix;
	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ProjMatrix;

	bool m_ContentLoaded;

	float m_toggleCooldown;
};

