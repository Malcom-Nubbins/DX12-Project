#include "UploadBuffer.h"
#include "../Application.h"
#include "../Globals/Helpers.h"
#include "../Globals/d3dx12.h"
#include <new>

UploadBuffer::UploadBuffer(size_t pageSize)
	: m_PageSize(pageSize)
{
}

UploadBuffer::~UploadBuffer()
{
}

UploadBuffer::Allocation UploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (sizeInBytes > m_PageSize)
	{
		throw std::bad_alloc();
	}

	if (!m_CurrentPage || !m_CurrentPage->HasSpace(sizeInBytes, alignment))
	{
		m_CurrentPage = RequestPage();
	}

	return m_CurrentPage->Allocate(sizeInBytes, alignment);
}

void UploadBuffer::Reset()
{
	m_CurrentPage = nullptr;
	m_AvailablePages = m_PagePool;

	for (auto page : m_AvailablePages)
	{
		page->Reset();
	}
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
	std::shared_ptr<Page> page;
	if (!m_AvailablePages.empty())
	{
		page = m_AvailablePages.front();
		m_AvailablePages.pop_front();
	}
	else
	{
		page = std::make_shared<Page>(m_PageSize);
		m_PagePool.push_back(page);
	}

	return page;
}

UploadBuffer::Page::Page(size_t sizeInBytes)
	: m_PageSize(sizeInBytes)
	, m_Offset(0)
	, m_cpuPtr(nullptr)
	, m_gpuPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
	auto device = Application::Get().GetDevice();

	const CD3DX12_HEAP_PROPERTIES UploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	const CD3DX12_RESOURCE_DESC UploadBuffer(CD3DX12_RESOURCE_DESC::Buffer(m_PageSize));

	ThrowIfFailed(device->CreateCommittedResource(
		&UploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&UploadBuffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&m_resource)));

	m_gpuPtr = m_resource->GetGPUVirtualAddress();
	m_resource->Map(0, nullptr, &m_cpuPtr);
}

UploadBuffer::Page::~Page()
{
	m_resource->Unmap(0, nullptr);
	m_cpuPtr = nullptr;
	m_gpuPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
	size_t alignedSize = Math::AlignUp(sizeInBytes, alignment);
	size_t alignedOffset = Math::AlignUp(m_Offset, alignment);

	return alignedOffset + alignedSize <= m_PageSize;
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (!HasSpace(sizeInBytes, alignment))
	{
		throw std::bad_alloc();
	}

	size_t alignedSize = Math::AlignUp(sizeInBytes, alignment);
	m_Offset = Math::AlignUp(m_Offset, alignment);

	Allocation alloc;
	alloc.CPU = static_cast<uint8_t*>(m_cpuPtr) + m_Offset;
	alloc.GPU = m_gpuPtr + m_Offset;

	m_Offset += alignedSize;

	return alloc;
}

void UploadBuffer::Page::Reset()
{
	m_Offset = 0;
}
