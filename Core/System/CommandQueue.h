#pragma once
#include "../Globals/stdafx.h"

class CommandQueue
{
public:
	CommandQueue(D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandQueue();

	ComPtr<ID3D12GraphicsCommandList2> GetCommandList();
	ComPtr<ID3D12CommandQueue> GetCommandQueue() const;

	uint64_t ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList);

	uint64_t Signal();
	bool IsFenceComplete(uint64_t fenceValue);
	void WaitForFenceValue(uint64_t fenceValue);
	void Flush();

protected:
	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
	ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator);

private:

	struct CommandAllocatorEntry
	{
		uint64_t fenceValue;
		ComPtr<ID3D12CommandAllocator> commandAllocator;
	};

	typedef std::queue<CommandAllocatorEntry> CommandAllocatorQueue;
	typedef std::queue<ComPtr<ID3D12GraphicsCommandList2>> CommandListQueue;

	D3D12_COMMAND_LIST_TYPE m_CommandListType;
	ComPtr<ID3D12CommandQueue> m_CommandQueue;
	ComPtr<ID3D12Fence> m_Fence;
	HANDLE m_FenceEvent;
	uint64_t m_FenceValue;

	CommandAllocatorQueue m_CommandAllocatorQueue;
	CommandListQueue m_CommandListQueue;
};

