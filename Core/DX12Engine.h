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
	virtual bool UnloadContent() override;

protected:
	virtual void OnUpdate() override;
	virtual void OnRender() override;

	virtual void OnKeyPressed() override;
	virtual void OnMouseWheel() override;
	virtual void OnResize(UINT width, UINT height) override;

private:
	void TransitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
		ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	void ClearRTV(ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColour);

	uint64_t m_FenceValues[AppWindow::BufferCount] = {};
};

