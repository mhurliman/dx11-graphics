//--------------------------------------------------------------------------------------
// main.cpp
//
// EA Runtime Technology Group (RTG)
// Copyright (C) Electronic Arts. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "D3DApp.h"

std::unique_ptr<D3DApp> g_d3dApp;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        g_d3dApp->HandleInput(hwnd, uMsg, wParam, lParam); // Pass user input to our app for handling
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    ////
    // Create a window

    const wchar_t className[] = L"RTG Graphics Class";
    const wchar_t* appName = L"Dx11HelloWorld";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,                            // Optional window styles
        className,                    // Window class
        appName,                      // Window text
        WS_OVERLAPPEDWINDOW,          // Window style
        CW_USEDEFAULT, CW_USEDEFAULT, // Position
        CW_USEDEFAULT, CW_USEDEFAULT, // Size
        nullptr,                      // Parent window
        nullptr,                      // Menu
        hInstance,                    // Instance handle
        nullptr                       // Additional application data
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    ////
    // Initialize our app

    g_d3dApp.reset(new D3DApp);
    g_d3dApp->Init(hwnd);

    ////
    // Run main game loop

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        g_d3dApp->Update();     // Update game logic
        g_d3dApp->Draw();       // Draw game state each frame
        g_d3dApp->Present();    // Flip back buffer to the front
    }

    ////
    // Cleanup app

    g_d3dApp->Shutdown();

    return 0;
}
