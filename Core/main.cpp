#include "Globals/stdafx.h"
#include "Application.h"
#include "DX12Engine.h"

#include <Shlwapi.h>
#include <dxgidebug.h>

void ReportLiveObjects()
{
	IDXGIDebug1* dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	dxgiDebug->Release();
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	int retCode = 0;

	WCHAR path[MAX_PATH];
	HMODULE hModule = GetModuleHandleW(nullptr);
	if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
	{
		PathRemoveFileSpecW(path);
		SetCurrentDirectoryW(path);
	}

	Application::Create(hInstance);
	{
		std::shared_ptr<DX12Engine> dx12Engine = std::make_shared<DX12Engine>(L"DX12 Engine", 1280, 720);
		retCode = Application::Get().Run(dx12Engine);
	}

	Application::Destroy();

	atexit(&ReportLiveObjects);

	return retCode;
}