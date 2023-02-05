#include "DescriptorAllocation.h"
#include "DescriptorAllocatorPage.h"
#include "../../Application.h"

#include "../../Globals/stdafx.h"
#include "../../Globals/Helpers.h"

DescriptorAllocation::DescriptorAllocation()
	: m_Descriptor{ 0 }
	, m_NumHandles(0)
	, m_DescriptorSize(0)
	, m_Page(nullptr)
{
}

DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle, uint32_t numHandles, uint32_t descriptorSize, std::shared_ptr<DescriptorAllocatorPage> page)
	: m_Descriptor(descriptorHandle)
	, m_NumHandles(numHandles)
	, m_DescriptorSize(descriptorSize)
	, m_Page(page)
{
}

DescriptorAllocation::~DescriptorAllocation()
{
	Free();
}

DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& allocation) noexcept
	: m_Descriptor(allocation.m_Descriptor)
	, m_NumHandles(allocation.m_NumHandles)
	, m_DescriptorSize(allocation.m_DescriptorSize)
	, m_Page(allocation.m_Page)
{
	allocation.m_Descriptor.ptr = 0;
	allocation.m_NumHandles = 0;
	allocation.m_DescriptorSize = 0;
}

DescriptorAllocation& DescriptorAllocation::operator=(DescriptorAllocation&& other) noexcept
{
	Free();

	m_Descriptor = other.m_Descriptor;
	m_NumHandles = other.m_NumHandles;
	m_DescriptorSize = other.m_DescriptorSize;
	m_Page = std::move(other.m_Page);

	other.m_Descriptor.ptr = 0;
	other.m_NumHandles = 0;
	other.m_DescriptorSize = 0;

	return *this;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetDescriptorHandle(uint32_t offset) const
{
	assert(offset < m_NumHandles);
	return { m_Descriptor.ptr + static_cast<SIZE_T>(static_cast<SIZE_T>(m_DescriptorSize) * static_cast<SIZE_T>(offset)) };
}

void DescriptorAllocation::Free()
{
	if (!IsNull() && m_Page)
	{
		m_Page->Free(std::move(*this), Application::GetFrameCount());

		m_Descriptor.ptr = 0;
		m_NumHandles = 0;
		m_DescriptorSize = 0;
		m_Page.reset();
	}
}


