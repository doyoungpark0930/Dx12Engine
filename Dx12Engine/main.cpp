#include "pch.h"
#include "Renderer.h"
#include "winApp.h"

#pragma comment(lib, "dxguid.lib")

//directx 12 Agility sdk
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 4; }

extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

void CreateConsole();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); //�޸� ���� ���(com��ü�� �ȵ�)
#endif 

	CreateConsole();
	WinApp::Run(hInstance, nCmdShow, L"DoYeong's Engine", 1280, 720);

#ifdef _DEBUG
	_ASSERT(_CrtCheckMemory()); //�� �ջ� ���
#endif

	return 0;

}

void CreateConsole()
{
	AllocConsole();  // �ܼ� â ����

	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);  // printf ��� �����ϰ� ����
}