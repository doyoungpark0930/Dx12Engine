#include "pch.h"
#include "Renderer.h"
#include "winApp.h"

#pragma comment(lib, "dxguid.lib")

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 4; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

void CreateConsole();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif 

	CreateConsole();
	WinApp::Run(hInstance, nCmdShow, L"DoYeong's Engine");

#ifdef _DEBUG
	_ASSERT(_CrtCheckMemory());
#endif

	return 0;

}

void CreateConsole()
{
	AllocConsole();  // 콘솔 창 생성

	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);  // printf 사용 가능하게 연결
}