//
// D3DApp.h
//

#pragma once

#include <d3d11_4.h>
#include <DirectXMath.h>
#include <wrl.h>

#include "MeshLoader.h"

using Microsoft::WRL::ComPtr;

class D3DApp
{
public:
    D3DApp()
        : m_isRunning(true)
        , m_width{}
        , m_height{}
        , m_frameIndex{}
        , m_objectPosition{}
        , m_objectRotation{}
        , m_objectScale(0.7f)
        , m_objectRotationSpeed(10.0f)
        , m_objectColor(0.6f, 0.7f, 0.1f)
        , m_objectShininess(256.0f)
        , m_cameraFocus{}
        , m_cameraDistance(5.0f)
        , m_cameraPhi(60.0f)
        , m_cameraTheta{}
        , m_cameraRotateRate(7.0f)
        , m_lightPosition(1.0f, 3.0f, 0.0f)
        , m_lightColor(1.0f, 1.0f, 1.0f)
        , m_currPos{}
        , m_prevPos{}
        , m_indexCount{}
    { }

    ~D3DApp()
    {
        Shutdown();
    }

    bool    IsRunning() const { return m_isRunning; }

    void    Init(HWND hwnd);
    LRESULT HandleInput(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void    Update(float dt);
    void    Draw();
    void    Present();
    void    Shutdown() {}

private:
    void    InitDevice(HWND hwnd);
    void    ResizeResources(UINT width, UINT height);
    void    InitResources();

private:
    bool                            m_isRunning;
    UINT                            m_width;
    UINT                            m_height;
    UINT                            m_frameIndex;

    ////
    // Application state

    // Object properties
    DirectX::XMFLOAT3               m_objectPosition;
    float                           m_objectRotation;
    float                           m_objectScale;
    float                           m_objectRotationSpeed;
    DirectX::XMFLOAT3               m_objectColor;
    float                           m_objectShininess;

    Mesh                            m_meshData;

    // Orbital camera properties (spherical coordinates)
    DirectX::XMFLOAT3               m_cameraFocus;
    float                           m_cameraDistance;
    float                           m_cameraPhi;
    float                           m_cameraTheta;
    float                           m_cameraRotateRate;

    DirectX::XMFLOAT3               m_lightPosition;
    DirectX::XMFLOAT3               m_lightColor;

    // User interaction
    DirectX::XMFLOAT2                m_currPos;
    DirectX::XMFLOAT2                m_prevPos;


    ////
    // ID3D11 Resources are ref-counted, so we utilize Microsoft::WRL::ComPtr to manage lifetime

    // Core device API objects
    ComPtr<ID3D11Device>            m_device;
    ComPtr<ID3D11DeviceContext>     m_deviceContext;

    ComPtr<IDXGISwapChain1>         m_swapChain;

    // Resources
    ComPtr<ID3D11Texture2D>         m_backBuffer;
    ComPtr<ID3D11RenderTargetView>  m_backBufferRTV;

    ComPtr<ID3D11Texture2D>         m_depthBuffer;
    ComPtr<ID3D11DepthStencilView>  m_depthBufferDSV;

    ComPtr<ID3D11Buffer>            m_vertexBuffer;
    ComPtr<ID3D11Buffer>            m_indexBuffer;
    UINT                            m_indexCount;

    // Input layout & shaders
    ComPtr<ID3D11InputLayout>       m_inputLayout;
    ComPtr<ID3D11VertexShader>      m_vertexShader;
    ComPtr<ID3D11PixelShader>       m_pixelShader;

    ComPtr<ID3D11Buffer>            m_uploadBuffer;
    ComPtr<ID3D11Buffer>            m_constantBuffer[2]; // Double-buffered - one for each frame

    // Graphics state
    ComPtr<ID3D11RasterizerState>   m_rasterizerState;
    ComPtr<ID3D11DepthStencilState> m_depthStencilState;
    ComPtr<ID3D11BlendState>        m_blendState;
};
