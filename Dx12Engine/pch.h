#pragma once

#include <windows.h>
#include <stdio.h>
#include <crtdbg.h>
#include <stdexcept>

#include <DirectXMath.h>
#include <d3d12.h>
#include <dxgi1_6.h> 
#include <d3dcompiler.h> // 셰이더 컴파일 시
#include <dxgidebug.h>
#include "d3dx12.h"


#ifdef _DEBUG
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif