//
// main.cpp
//

#include "pch.h"

#include "D3DApp.h"

using namespace std::chrono;

std::unique_ptr<D3DApp> g_d3dApp;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (g_d3dApp)
    {
        return g_d3dApp->HandleInput(hwnd, uMsg, wParam, lParam); // Pass user input to our app for handling
    }
    else
    {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    ////
    // Create a window

    const wchar_t className[] = L"RTG Graphics Class";
    const wchar_t* appName = L"Dx11MeshViewer";

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

    auto prev = high_resolution_clock::now();

    while (g_d3dApp->IsRunning())
    {
        // Servic the Windows message queue
        MSG msg {};
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Compute delta time
        auto curr = high_resolution_clock::now();
        auto dt = duration<float>(curr - prev).count();
        prev = curr;

        g_d3dApp->Update(dt);     // Update game logic
        g_d3dApp->Draw();       // Draw game state each frame
        g_d3dApp->Present();    // Flip back buffer to the front
    }

    ////
    // Cleanup app

    g_d3dApp->Shutdown();

    return 0;
}
