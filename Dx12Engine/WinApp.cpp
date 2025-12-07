#include "pch.h"
#include "Renderer.h"
#include "Camera.h"
#include "DXUtil.h"
#include "winApp.h"
#include <iostream>
Renderer* WinApp::m_renderer = nullptr;
HWND WinApp::m_hwnd = nullptr;
DWORD	g_FrameCount = 0;
ULONGLONG g_PrvFrameCheckTick = 0;
ULONGLONG g_LastFrameTick = 0;

float m_cursorNdcX = 0;
float m_cursorNdcY = 0;
float deltaTime = 0.0f;

bool IsCursorMode = false;
bool m_warping = false;
bool m_cursorHidden = false;

bool keyPressed[256] =
{
    false,
};




int WinApp::Run(HINSTANCE hInstance, int nCmdShow, const wchar_t CLASS_NAME[], UINT width, UINT height)
{
    m_renderer = new Renderer(width, height);
    camera.SetAspect(m_renderer->GetAspect());
    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);
    WinApp::m_hwnd = nullptr;

    RECT rc = { 0, 0, m_renderer->GetWidth(), m_renderer->GetHeight()}; // 원하는 클라이언트 영역 크기
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE);

    // Create the window.
    m_hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"DoYeong's Engine",    // Window text
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,            // 윈도우 크기 조절 막음

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (m_hwnd == NULL)
    {
        return 0;
    }

    m_renderer->OnInit();

    ShowWindow(m_hwnd, nCmdShow);
   
  
    // Run the message loop.
    int cnt = 0;
    MSG msg = { };
    g_LastFrameTick = GetTickCount64(); //g_LastFrameTick 초기화
    while (true)
    {
        //메시지 입력이 무한한 것 같지만, 메시지 입력이 프레임 속도를 막지 않도록 설계됨
        //MouseMove가 계속 실행되는 것 같지만, MouseMove 메시지 -> 한 프레임 -> MouseMove메시지 -> 한 프레임 ..
        //하지만 MouseMove에 비해, keyboard는 메시지 입력 속도가 느려서 메시지 -> 3프레임 ->메시지 이런 경우도 있음
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_FrameCount++;
            ULONGLONG CurTick = GetTickCount64();
            deltaTime = (CurTick - g_LastFrameTick) / 1000.0f; // 밀리초 -> 초
            g_LastFrameTick = CurTick;

            m_renderer->Update(deltaTime);
            m_renderer->Render();
            
            if (CurTick - g_PrvFrameCheckTick > 1000) //1000밀리초 = 1초
            {
                g_PrvFrameCheckTick = CurTick;

                WCHAR wchTxt[64];
                swprintf_s(wchTxt, L"FPS:%u", g_FrameCount);
                SetWindowText(m_hwnd, wchTxt);

                g_FrameCount = 0;
            }
            camera.IsMouseMoving = false;
        }
    };

    if (m_renderer)
    {
        delete m_renderer;
        m_renderer = nullptr;
    }


    return 0;
}

POINT GetWindowCenter(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    POINT center;
    center.x = (rc.right - rc.left) / 2;
    center.y = (rc.bottom - rc.top) / 2;

    ClientToScreen(hwnd, &center); // 스크린 좌표로 변환
    return center;
}

void UpdateCursorState()
{
    if (!IsCursorMode && !m_cursorHidden)
    {
        while (ShowCursor(FALSE) >= 0); // 확실히 숨김
        m_cursorHidden = true;
    }
    else if (IsCursorMode && m_cursorHidden)
    {
        while (ShowCursor(TRUE) < 0); // 확실히 보이게
        m_cursorHidden = false;
    }
}
LRESULT CALLBACK WinApp::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        //All painting occurs here, between BeginPaint and EndPaint.

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);
    }
    case WM_KEYDOWN:
    {
        //std::cout << "WM_KEYDOWN " << (int)wParam << std::endl;
        // 키보드가 눌린 상태인지 아닌지 저장
        keyPressed[wParam] = true;
        if (wParam == 'F') { // F키 일인칭 시점
            camera.IsFirstPersonView = !camera.IsFirstPersonView;
        }
        if (wParam == 27) //ESC
        {
            IsCursorMode = !IsCursorMode;
        }
        break;
    }

    case WM_KEYUP:
        // 키보드가 눌린 상태인지 아닌지 저장
        keyPressed[wParam] = false;
        break;

    case WM_MOUSEMOVE:
        if (m_warping) {
            m_warping = false;
            break; // 워프 때문에 발생한 이벤트는 무시
        }
        UpdateCursorState();
        if (!IsCursorMode)
        {
            POINT center = GetWindowCenter(hwnd);
            SetCursorPos(center.x, center.y);
            OnMouseMove(LOWORD(lParam), HIWORD(lParam));
            m_warping = true;
        }

        camera.IsMouseMoving = true;
        break;

    return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void WinApp::OnMouseMove(int mouseX, int mouseY) {

    int dx = mouseX - 640;
    int dy = mouseY - 360;
  
    float sensitivity = 0.002f;

    camera.UpdateMouseDelta(dx * sensitivity, -dy * sensitivity);
}
