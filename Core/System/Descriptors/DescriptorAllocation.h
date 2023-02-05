#pragma once
#include <d3d12.h>
#include <cstdint>
#include <memory>

class DescriptorAllocatorPage;

class DescriptorAllocation
{
public:
	DescriptorAllocation();
	DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle, uint32_t numHandles, uint32_t descriptorSize, std::shared_ptr<DescriptorAllocatorPage> page);
	~DescriptorAllocation();

	DescriptorAllocation(const DescriptorAllocation&) = delete;
	DescriptorAllocation& operator=(const DescriptorAllocation&) = delete;

	DescriptorAllocation(DescriptorAllocation&& allocation) noexcept;
	DescriptorAllocation& operator=(DescriptorAllocation&& other) noexcept;

	bool IsNull() const { return m_Descriptor.ptr == 0; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t offset = 0) const;

	uint32_t GetNumHandles() const { return m_NumHandles; }

	std::shared_ptr<DescriptorAllocatorPage> GetDescriptorAllocationPage() const { return m_Page; }

private:
	void Free();

	D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptor;
	uint32_t m_NumHandles;
	uint32_t m_DescriptorSize;

	std::shared_ptr<DescriptorAllocatorPage> m_Page;
};

