//
// D3DApp.cpp
//

#include "pch.h"
#include <DirectXMath.h>
#include <d3dcompiler.h>

#include "D3DApp.h"

using DirectX::XMFLOAT3;

// Vertex format for screen-space triangle
// 32-bit per component XYZ position & RGB color
struct PosColorVertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Color;
};


void D3DApp::Init(HWND hwnd)
{
    // Grab the window's client size
    RECT rect{};
    GetClientRect(hwnd, &rect);

    UINT width = static_cast<UINT>(rect.right - rect.left);
    UINT height = static_cast<UINT>(rect.bottom - rect.top);

    InitDevice(hwnd);
    ResizeResources(width, height);
    InitResources();
}

void D3DApp::InitDevice(HWND hwnd)
{
    ////
    // Create the D3D device & context

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG; // Use the validated D3D driver in debug mode
#endif

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    ThrowIfFailed(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, &featureLevel, 1, D3D11_SDK_VERSION, m_device.ReleaseAndGetAddressOf(), nullptr, m_deviceContext.ReleaseAndGetAddressOf()));


    ///
    // Initialize the DXGI objects which enumerate our video adapters, monitors, and provides access to the swap chain buffer

    // DXGIDevice - provides interface for querying video adapters (and other stuff we don't care about)
    ComPtr<IDXGIDevice> dxgiDevice;
    ThrowIfFailed(m_device->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(dxgiDevice.ReleaseAndGetAddressOf())));

    // DXGIAdapter - encapsulates the video adapter (GPU)
    ComPtr<IDXGIAdapter> dxgiAdapter;
    ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.ReleaseAndGetAddressOf()));

    // DXGIFactory - interface for managing window behavior and swap chain creation/access
    ComPtr<IDXGIFactory2> dxgiFactory;
    ThrowIfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(dxgiFactory.ReleaseAndGetAddressOf())));


    ////
    // Specify desired swap chain behavior and back buffer pixel format

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc {};
    swapChainDesc.Width              = 0;                               // Use window width
    swapChainDesc.Height             = 0;                               // Use window height
    swapChainDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;      // 32-bpp RGBA format
    swapChainDesc.Stereo             = FALSE;
    swapChainDesc.SampleDesc.Count	 = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount        = 2;                               // Double-buffering
    swapChainDesc.Scaling            = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;   // Prefer DXGI_SWAP_EFFECT_FLIP_DISCARD
    swapChainDesc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags              = 0;

    // DXGISwapChain - API object providing access to swap chain buffers
    ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(m_device.Get(), hwnd, &swapChainDesc, nullptr, nullptr, m_swapChain.ReleaseAndGetAddressOf()));
}

void D3DApp::ResizeResources(UINT width, UINT height)
{
    m_backBufferRTV.Reset();
    m_backBuffer.Reset();

    if (width != m_width || height != m_height)
    {
        // The window size has changed - we need to resize the swap buffers and reacquire them
        m_width = width;
        m_height = height;

        ThrowIfFailed(m_swapChain->ResizeBuffers(2, m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
    }

    // ID3D11Texture2D pointing to the Swap Chain back buffer
    ThrowIfFailed(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(m_backBuffer.ReleaseAndGetAddressOf())));


    ////
    // Create render target view of back buffer, and the depth buffer w/ depth-stencil view

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // sRGB color writes
    rtvDesc.Texture2D.MipSlice = 0;

    ThrowIfFailed(m_device->CreateRenderTargetView(m_backBuffer.Get(), &rtvDesc, m_backBufferRTV.ReleaseAndGetAddressOf()));
}

void D3DApp::InitResources()
{
    ////
    // Upload the screen-space triangle vertex data to a GPU buffer

    // Three vertices - counter-clockwise winding
    PosColorVertex VertexData[] =
    {
        { { 0.0f,  0.7f, 0.0f }, { 1.0f, 0.0f, 0.0f } }, // Top vertex
        { {-0.4f, -0.7f, 0.0f }, { 0.0f, 0.0f, 1.0f } }, // Bottom-left vertex
        { { 0.4f, -0.7f, 0.0f }, { 0.0f, 1.0f, 0.0f } }, // Bottom-right vertex
    };

    D3D11_BUFFER_DESC vertexBufferDesc {};
    vertexBufferDesc.ByteWidth = sizeof(VertexData);
    vertexBufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData {};
    vertexData.pSysMem = VertexData;
    ThrowIfFailed(m_device->CreateBuffer(&vertexBufferDesc, &vertexData, m_vertexBuffer.ReleaseAndGetAddressOf()));


    ////
    // Load precompiled shader blobs from file (automatically compiled via Visual Studio & written to executable directory)

    // Vertex Shader - shader type, shader model, & entrypoint specified in Visual Studio file properties
    ComPtr<ID3DBlob> vsBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"PassThruVS.cso", vsBlob.ReleaseAndGetAddressOf()));
    ThrowIfFailed(m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_vertexShader.ReleaseAndGetAddressOf()));

    // Pixel Shader - shader type, shader model, & entrypoint specified in Visual Studio file properties
    ComPtr<ID3DBlob> psBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"PassThruPS.cso", psBlob.ReleaseAndGetAddressOf()));
    ThrowIfFailed(m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_pixelShader.ReleaseAndGetAddressOf()));


    ////
    // Declare the vertex layout - MUST align with declared vertex format in the Vertex Shader

    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ThrowIfFailed(m_device->CreateInputLayout(inputElementDesc, _countof(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_inputLayout.ReleaseAndGetAddressOf()));


    ////
    // Create our state which configures the fixed-function graphics pipeline

    // Rasterizer State - solid fill mode & disable primitive culling
    D3D11_RASTERIZER_DESC rasterizerDesc {};
    rasterizerDesc.FillMode              = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode              = D3D11_CULL_NONE;
    rasterizerDesc.FrontCounterClockwise = true;

    ThrowIfFailed(m_device->CreateRasterizerState(&rasterizerDesc, m_rasterizerState.ReleaseAndGetAddressOf()));

    // Depth-stencil State - disable depth testing & writes
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc {};
    depthStencilDesc.DepthEnable    = false;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc      = D3D11_COMPARISON_ALWAYS;

    ThrowIfFailed(m_device->CreateDepthStencilState(&depthStencilDesc, m_depthStencilState.ReleaseAndGetAddressOf()));

    // Blend State - disable blend
    D3D11_BLEND_DESC blendDesc {};
    blendDesc.IndependentBlendEnable                = false;
    blendDesc.AlphaToCoverageEnable                 = false;
    blendDesc.RenderTarget[0].BlendEnable           = false;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ThrowIfFailed(m_device->CreateBlendState(&blendDesc, m_blendState.ReleaseAndGetAddressOf()));


    ////
    // Create RenderTargetView of back buffer

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc {};
    rtvDesc.ViewDimension       = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.Texture2D.MipSlice  = 0;

    ThrowIfFailed(m_device->CreateRenderTargetView(m_backBuffer.Get(), &rtvDesc, m_backBufferRTV.ReleaseAndGetAddressOf()));
}

void D3DApp::Draw()
{
    // Clear the render target to dark grey
    FLOAT backgroundColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
    m_deviceContext->ClearRenderTargetView(m_backBufferRTV.Get(), backgroundColor);

    // Bind the color render target
    ID3D11RenderTargetView* rtvs[] = { m_backBufferRTV.Get() };
    m_deviceContext->OMSetRenderTargets(1, rtvs, nullptr);

    D3D11_VIEWPORT viewport =
    {
        0.0f, 0.0f,                                                 // Top-left X, Y
        static_cast<float>(m_width), static_cast<float>(m_height),  // Width, Height
        0.0f, 1.0f                                                  // Min/Max Depth
    };
    m_deviceContext->RSSetViewports(1, &viewport);

    // Set the desired primitive topology and vertex layout
    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_deviceContext->IASetInputLayout(m_inputLayout.Get());

    // Bind the vertex buffer
    UINT stride = sizeof(PosColorVertex);
    UINT offset = 0;
    ID3D11Buffer* vbuffers[] = { m_vertexBuffer.Get() };
    m_deviceContext->IASetVertexBuffers(0, 1, vbuffers, &stride, &offset);

    // Bind vertex & pixel shader programs
    m_deviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // Bind fixed-function graphics pipeline state
    m_deviceContext->RSSetState(m_rasterizerState.Get());
    m_deviceContext->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    m_deviceContext->OMSetBlendState(m_blendState.Get(), nullptr, 0xffffffff);

    // Draw our three happy screen-space vertices (1 triangle)
    m_deviceContext->Draw(3, 0);
}

void D3DApp::Present()
{
    ThrowIfFailed(m_swapChain->Present(1, 0)); // Flip on VBlank (vsync refresh rate interval)
}


LRESULT D3DApp::HandleInput(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        m_isRunning = false;

        return 0;

    case WM_SIZE:
    {
        UINT width = (lParam & 0x0000ffff) >> 0;
        UINT height = (lParam & 0xffff0000) >> 16;

        ResizeResources(width, height);

        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}