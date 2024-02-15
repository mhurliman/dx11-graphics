//--------------------------------------------------------------------------------------
// D3DApp.h
//
// EA Runtime Technology Group (RTG)
// Copyright (C) Electronic Arts. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

using Microsoft::WRL::ComPtr;

class D3DApp
{
public:
    D3DApp()
        : m_width(0)
        , m_height(0)
    { }

    ~D3DApp()
    {
        Shutdown();
    }

    void Init(HWND hwnd);
    void HandleInput(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {}
    void Update() {}
    void Draw();
    void Present();
    void Shutdown();

private:
    void InitDevice(HWND hwnd);
    void InitResources();

private:
    UINT                            m_width;
    UINT                            m_height;

    ComPtr<ID3D11Device>            m_device;
    ComPtr<ID3D11DeviceContext>     m_deviceContext;

    ComPtr<IDXGISwapChain1>         m_swapChain;

    ComPtr<ID3D11Texture2D>         m_backBuffer;
    ComPtr<ID3D11RenderTargetView>  m_backBufferRTV;

    ComPtr<ID3D11Buffer>            m_vertexBuffer;
    ComPtr<ID3D11InputLayout>       m_inputLayout;
    ComPtr<ID3D11VertexShader>      m_vertexShader;
    ComPtr<ID3D11PixelShader>       m_pixelShader;

    ComPtr<ID3D11RasterizerState>   m_rasterizerState;
    ComPtr<ID3D11DepthStencilState> m_depthStencilState;
    ComPtr<ID3D11BlendState>        m_blendState;
};
