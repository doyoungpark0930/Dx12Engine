#pragma once

class Renderer;

extern bool keyPressed[256];


class WinApp
{
public:
    static int Run(HINSTANCE hInstance, int nCmdShow, const wchar_t CLASS_NAME[], UINT width, UINT height);
    static HWND GetHwnd() { return m_hwnd; }
    static Renderer* m_renderer;

protected:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static void OnMouseMove(int mouseX, int mouseY);

private:
    static HWND m_hwnd;

};