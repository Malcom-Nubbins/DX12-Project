#include "DX12Engine.h"
#include "Application.h"
#include "Globals/Helpers.h"
#include "System/CommandQueue.h"

using namespace DirectX;

struct VertexPosColour
{
	XMFLOAT3 Position;
	XMFLOAT3 Colour;
};

static VertexPosColour Vertices[8] =
{
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
	{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
	{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
	{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
	{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
	{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) },
};

static WORD Indices[36] =
{
	0, 1, 2,
	0, 2, 3,
	4, 6, 5,
	4, 7, 6,
	4, 5, 1,
	4, 1, 0,
	3, 2, 6,
	3, 6, 7,
	1, 5, 6,
	1, 6, 2,
	4, 0, 3,
	4, 3, 7
};

DX12Engine::DX12Engine(const std::wstring& name, UINT width, UINT height, bool vsync)
	: super(name, width, height, vsync)
	, m_ContentLoaded(false)
	, m_FOV(45.0f)
	, m_VertexBufferView()
	, m_IndexBufferView()
	, m_ProjMatrix(XMMatrixIdentity())
	, m_WorldMatrix(XMMatrixIdentity())
	, m_ViewMatrix(XMMatrixIdentity())
	, m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
	, m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
	, m_toggleCooldown(0.0f)
{
}

bool DX12Engine::LoadContent()
{
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	UpdateBufferResource(commandList, &m_VertexBuffer, &intermediateVertexBuffer, _countof(Vertices), sizeof(VertexPosColour), Vertices);

	m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = sizeof(Vertices);
	m_VertexBufferView.StrideInBytes = sizeof(VertexPosColour);

	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UpdateBufferResource(commandList, &m_IndexBuffer, &intermediateIndexBuffer, _countof(Indices), sizeof(WORD), Indices);

	m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.SizeInBytes = sizeof(Indices);
	m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));

	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"ColourVertexShader.cso", &vertexShaderBlob));

	ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(L"ColourPixelShader.cso", &pixelShaderBlob));

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
		featureData.HighestVersion, &rootSignatureBlob, &errorBlob));

	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = m_RootSignature.Get();
	pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
	{
		sizeof(PipelineStateStream), &pipelineStateStream
	};

	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState)));

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	m_ContentLoaded = true;

	ResizeDepthBuffer(GetClientWidth(), GetClientHeight());

	return true;
}

void DX12Engine::UnloadContent()
{
	m_ContentLoaded = false;
}

void DX12Engine::OnUpdate(UpdateEvent& e)
{
	static uint64_t frameCount = 0;
	static double totalTime = 0.0;

	super::OnUpdate(e);

	totalTime += e.ElapsedTime;
	frameCount++;

	float angle = static_cast<float>(e.TotalTime * 90.0);
	const XMVECTOR  rotationAxis = XMVectorSet(0, 1, 1, 0);
	m_WorldMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

	const XMVECTOR eyePos = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDir = XMVectorSet(0, 1, 0, 0);
	m_ViewMatrix = XMMatrixLookAtLH(eyePos, focusPoint, upDir);

	float aspectRatio = GetClientWidth() / static_cast<float>(GetClientHeight());
	m_ProjMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FOV), aspectRatio, 0.1f, 100.0f);

	if (m_toggleCooldown > 0.0f)
	{
		m_toggleCooldown -= static_cast<float>(e.ElapsedTime);
	}
}

void DX12Engine::OnRender(RenderEvent& e)
{
	super::OnRender(e);

	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();

	UINT currentBackBufferIndex = m_AppWindow->GetCurrentBackBufferIndex();
	auto backBuffer = m_AppWindow->GetCurrentBackBuffer();
	auto rtv = m_AppWindow->GetCurrentRenderTargetView();
	auto dsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

	// Clear render targets
	{
		TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		FLOAT clearColour[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		ClearRTV(commandList, rtv, clearColour);
		ClearDepth(commandList, dsv);
	}

	commandList->SetPipelineState(m_PipelineState.Get());
	commandList->SetGraphicsRootSignature(m_RootSignature.Get());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	commandList->IASetIndexBuffer(&m_IndexBufferView);

	commandList->RSSetViewports(1, &m_Viewport);
	commandList->RSSetScissorRects(1, &m_ScissorRect);

	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	XMMATRIX mvpMatrix = XMMatrixMultiply(m_WorldMatrix, m_ViewMatrix);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjMatrix);
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

	commandList->DrawIndexedInstanced(_countof(Indices), 1, 0, 0, 0);

	// Present
	{
		TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		
		m_FenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

		currentBackBufferIndex = m_AppWindow->Present();
		commandQueue->WaitForFenceValue(m_FenceValues[currentBackBufferIndex]);
	}
}

void DX12Engine::OnKeyPressed(KeyEvent& e)
{
	super::OnKeyPressed(e);

	switch (e.Key)
	{
	case KeyCode::Escape:
		Application::Get().Quit(0);
		break;

	case KeyCode::Enter:
		if (e.Alt)
		{
		case KeyCode::F11:
			if (m_toggleCooldown <= 0.0f)
			{
				m_AppWindow->ToggleFullscreen();
				m_toggleCooldown = 2.0f;
			}
			break;
		}
		break;
	case KeyCode::V:
		if (m_toggleCooldown <= 0.0f)
		{
			m_AppWindow->ToggleVSync();
			m_toggleCooldown = 2.0f;
		}
		break;
	}
}

void DX12Engine::OnMouseWheel(MouseWheelEvent& e)
{
	m_FOV -= e.WheelDelta;
	m_FOV = clamp(m_FOV, 12.0f, 90.0f);
}

void DX12Engine::OnResize(UINT width, UINT height)
{
	if (width != GetClientWidth() || height != GetClientHeight())
	{
		super::OnResize(width, height);

		m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));

		ResizeDepthBuffer(width, height);
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

void DX12Engine::ClearDepth(ComPtr<ID3D12GraphicsCommandList2> commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth)
{
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void DX12Engine::UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList2> commandList, ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	auto device = Application::Get().GetDevice();

	size_t bufferSize = numElements * elementSize;

	const CD3DX12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	const CD3DX12_RESOURCE_DESC Buffer(CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags));

	ThrowIfFailed(device->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&Buffer,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(destinationResource)));

	if (bufferData)
	{
		const CD3DX12_HEAP_PROPERTIES UploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC UploadBuffer(CD3DX12_RESOURCE_DESC::Buffer(bufferSize));

		ThrowIfFailed(device->CreateCommittedResource(
			&UploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&UploadBuffer,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(intermediateResource)));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(commandList.Get(), *destinationResource, *intermediateResource,
			0, 0, 1, &subresourceData);
	}
}

void DX12Engine::ResizeDepthBuffer(UINT width, UINT height)
{
	if (m_ContentLoaded)
	{
		Application::Get().Flush();

		width = std::max(1, static_cast<int>(width));
		height = std::max(1, static_cast<int>(height));

		auto device = Application::Get().GetDevice();

		D3D12_CLEAR_VALUE optimisedClearValue = {};
		optimisedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		optimisedClearValue.DepthStencil = { 1.0f, 0 };

		const CD3DX12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
		const CD3DX12_RESOURCE_DESC Tex2D(CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL));

		ThrowIfFailed(device->CreateCommittedResource(
			&HeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&Tex2D,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&optimisedClearValue,
			IID_PPV_ARGS(&m_DepthBuffer)));

		D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv, m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
	}
}
