#pragma once
#include "../Globals/stdafx.h"
#include "../Globals/Helpers.h"
#include <memory>
#include <deque>

class UploadBuffer
{
public: 
	struct Allocation
	{
		void* CPU;
		D3D12_GPU_VIRTUAL_ADDRESS GPU;
	};

	explicit UploadBuffer(size_t pageSize = _2MB);

	virtual ~UploadBuffer();

	size_t GetPageSize() const { return m_PageSize; }

	Allocation Allocate(size_t sizeInBytes, size_t alignment);

	void Reset();

private:
	struct Page
	{
		Page(size_t sizeInBytes);
		~Page();
		bool HasSpace(size_t sizeInBytes, size_t alignment) const;
		Allocation Allocate(size_t sizeInBytes, size_t alignment);
		void Reset();

	private:
		ComPtr<ID3D12Resource> m_resource;

		void* m_cpuPtr;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpuPtr;

		size_t m_PageSize;
		size_t m_Offset;
	};

	using PagePool = std::deque<std::shared_ptr<Page>>;

	std::shared_ptr<Page> RequestPage();

	PagePool m_PagePool;
	PagePool m_AvailablePages;

	std::shared_ptr<Page> m_CurrentPage;

	size_t m_PageSize;
};

