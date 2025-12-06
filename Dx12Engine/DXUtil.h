#pragma once

class DXUtil
{
public :
	static void GetResourcesPathName(const wchar_t* assetName, wchar_t* outBuffer, size_t bufSize);
	static wchar_t m_assetsResourcesPath[512];
};

enum class AnimType
{
	Idle = 0,
	WalkForward,
	RunForward,
	WalkBackward,
	WalkLeft,
	WalkRight,
	RunLeft,
	RunRight,

	// --- 숫자키로 호출되는 특수 동작들 ---
	Martelo,
	Boxing,
	JabCross,
	ForwardFlip
};

void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);

HRESULT SuballocateFromBuffer(UINT8* m_pDataCur, UINT8* m_pDataEnd, SIZE_T uSize, UINT uAlign);

HRESULT SetDataToUploadBuffer(
	UINT8** m_pDataCur,
	UINT8* m_pDataBegin,
	UINT8* m_pDataEnd,
	const void* pData,
	UINT bytesPerData,
	UINT dataCount,
	UINT alignment,
	UINT& byteOffset
);

SIZE_T Align(SIZE_T uLocation, SIZE_T uAlign);


AnimType GetMovementAnim(bool IsFirstPersonView);

AnimType GetSkillAnim();

AnimType GetAnimationType(bool IsFirstPersonView);

void ReverseIndices(UINT* indices, UINT indicesNum);

Matrix GetObjectWorldMatrix(const ObjectState& state);

char* MakeFilePath(const wchar_t* basePath, const char* subFolder, const char* fileName);

template <typename T>
const T& clamp(const T& v, const T& lo, const T& hi)
{
	return (v < lo) ? lo : (hi < v) ? hi : v;
}

void InitActionKey(char (&actionKey)[32]);

bool IsNumberKeyPressed(const bool(&keyPressed)[256]);