#include "pch.h"
#include "winApp.h"
#include "Renderer.h"

Renderer* WinApp::m_renderer = nullptr;
HWND WinApp::m_hwnd = nullptr;

int WinApp::Run(HINSTANCE hInstance, int nCmdShow, const wchar_t CLASS_NAME[], UINT width, UINT height)
{
    m_renderer = new Renderer(width, height);

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
    while (true)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            m_renderer->OnUpdate();
            m_renderer->OnRender();
        }
    };

    if (m_renderer)
    {
        delete m_renderer;
        m_renderer = nullptr;
    }


    return 0;
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

        // All painting occurs here, between BeginPaint and EndPaint.

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);
    }
    return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

