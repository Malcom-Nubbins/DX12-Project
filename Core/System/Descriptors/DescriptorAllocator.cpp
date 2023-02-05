#include "DescriptorAllocator.h"
#include "DescriptorAllocatorPage.h"
#include "../../Globals/stdafx.h"
#include "../../Globals/Helpers.h"

DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap)
	: m_HeapType(type)
	, m_NumDescriptorsPerHeap(numDescriptorsPerHeap)
{
}

DescriptorAllocator::~DescriptorAllocator()
{
}

DescriptorAllocation DescriptorAllocator::Allocate(uint32_t numDescriptors)
{
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	DescriptorAllocation alloc;
	for (auto iter = m_AvailableHeaps.begin(); iter != m_AvailableHeaps.end(); ++iter)
	{
		auto allocatorPage = m_HeapPool[*iter];
		alloc = allocatorPage->Allocate(numDescriptors);
		if (allocatorPage->NumFreeHandles() == 0)
		{
			iter = m_AvailableHeaps.erase(iter);
		}

		if (!alloc.IsNull())
		{
			break;
		}
	}

	if (alloc.IsNull())
	{
		m_NumDescriptorsPerHeap = std::max(m_NumDescriptorsPerHeap, numDescriptors);
		auto newPage = CreateAllocatorPage();
		alloc = newPage->Allocate(numDescriptors);
	}

	return alloc;
}

void DescriptorAllocator::ReleaseStaleDescriptors(uint64_t frameNumber)
{
	std::lock_guard<std::mutex> lock(m_AllocationMutex);
	for (size_t i = 0; i < m_HeapPool.size(); ++i)
	{
		auto page = m_HeapPool[i];
		page->ReleaseStaleDescriptors(frameNumber);

		if (page->NumFreeHandles() > 0)
		{
			m_AvailableHeaps.insert(i);
		}
	}
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
{
	auto newPage = std::make_shared<DescriptorAllocatorPage>(m_HeapType, m_NumDescriptorsPerHeap);

	m_HeapPool.emplace_back(newPage);
	m_AvailableHeaps.insert(m_HeapPool.size() - 1);
	return newPage;
}
