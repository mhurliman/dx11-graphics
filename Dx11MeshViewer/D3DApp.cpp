//--------------------------------------------------------------------------------------
// D3DApp.cpp
//
// EA Runtime Technology Group (RTG)
// Copyright (C) Electronic Arts. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "D3DApp.h"

#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "MeshLoader.h"


using namespace DirectX;

using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;
using DirectX::XMFLOAT4X4;
using DirectX::XMVECTOR;
using DirectX::XMMATRIX;

// Vertex format for screen-space triangle
// 32-bit per component XYZ position & unit normal vector
struct PosNormalVertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
};

// Shader constant buffer
struct AppShaderConstants
{
    // Transforms
    XMFLOAT4X4 World;
    XMFLOAT4X4 WorldViewProjection;

    // Object material properties
    XMFLOAT3 ObjectColor;
    float    ObjectShininess;

    // Light properties
    XMFLOAT4 LightPositionWS;
    XMFLOAT4 LightColor;

    // Camera properties
    XMFLOAT4 CameraPositionWS;
};


void D3DApp::Init(HWND hwnd)
{
    // Grab the window's client size
    RECT rect{};
    GetClientRect(hwnd, &rect);

    UINT width  = static_cast<UINT>(rect.right - rect.left);
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

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = 0;                                // Use window width
    swapChainDesc.Height = 0;                               // Use window height
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;      // 32-bpp RGBA format
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;                               // Double-buffering
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;   // Prefer DXGI_SWAP_EFFECT_FLIP_DISCARD
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    // DXGISwapChain - API object providing access to swap chain buffers
    ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(m_device.Get(), hwnd, &swapChainDesc, nullptr, nullptr, m_swapChain.ReleaseAndGetAddressOf()));
}

void D3DApp::ResizeResources(UINT width, UINT height)
{
    m_backBufferRTV.Reset();
    m_backBuffer.Reset();

    m_depthBufferDSV.Reset();
    m_depthBuffer.Reset();

    if (width != m_width || height != m_height)
    {
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

    D3D11_TEXTURE2D_DESC depthBufferDesc{};
    depthBufferDesc.Format = DXGI_FORMAT_D32_FLOAT; // 32-bit depth; no stencil plane
    depthBufferDesc.Width = m_width;
    depthBufferDesc.Height = m_height;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 }; // MSAA Settings - no MSAA
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.MiscFlags = 0;
    depthBufferDesc.CPUAccessFlags = 0;

    ThrowIfFailed(m_device->CreateTexture2D(&depthBufferDesc, nullptr, m_depthBuffer.ReleaseAndGetAddressOf()));
    ThrowIfFailed(m_device->CreateDepthStencilView(m_depthBuffer.Get(), nullptr, m_depthBufferDSV.ReleaseAndGetAddressOf())); // Not specifying D3D11_DEPTH_STENCIL_VIEW_DESC result in a default behavior
}

void D3DApp::InitResources()
{
    ////
    // Load the mesh from file and upload vertex & index buffers to the GPU

    Mesh loadedMesh;
    ThrowIfFailed(LoadMesh("teapot.obj", loadedMesh));

    m_indexCount = static_cast<UINT>(loadedMesh.IndexBuffer.size());

    XMVECTOR min = DirectX::XMLoadFloat3(&loadedMesh.BoundsMin);
    XMVECTOR max = DirectX::XMLoadFloat3(&loadedMesh.BoundsMax);
 
    DirectX::XMStoreFloat3(&m_cameraFocus, (max + min) / 2 * m_objectScale);


    D3D11_BUFFER_DESC vertexBufferDesc {};
    vertexBufferDesc.ByteWidth = static_cast<UINT>(loadedMesh.VertexBuffer.size() * sizeof(float));
    vertexBufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vertexData {};
    vertexData.pSysMem = loadedMesh.VertexBuffer.data();

    ThrowIfFailed(m_device->CreateBuffer(&vertexBufferDesc, &vertexData, m_vertexBuffer.ReleaseAndGetAddressOf()));


    D3D11_BUFFER_DESC indexBufferDesc {};
    indexBufferDesc.ByteWidth = m_indexCount * sizeof(uint32_t);
    indexBufferDesc.Usage     = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA indexData {};
    indexData.pSysMem = loadedMesh.IndexBuffer.data();

    ThrowIfFailed(m_device->CreateBuffer(&indexBufferDesc, &indexData, m_indexBuffer.ReleaseAndGetAddressOf()));


    ////
    // Create the constant buffer(s) and the staging buffer resources

    D3D11_BUFFER_DESC cbDesc {};
    cbDesc.ByteWidth = (sizeof(AppShaderConstants) + 255) & ~255; // Force up to next multiple of 256
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.Usage     = D3D11_USAGE_DEFAULT;

    for (int i = 0; i < 2; ++i)
    {
        ThrowIfFailed(m_device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer[i].ReleaseAndGetAddressOf()));
    }

    D3D11_BUFFER_DESC uploadDesc {};
    uploadDesc.ByteWidth      = cbDesc.ByteWidth;
    uploadDesc.Usage          = D3D11_USAGE_STAGING;
    uploadDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ThrowIfFailed(m_device->CreateBuffer(&uploadDesc, nullptr, m_uploadBuffer.ReleaseAndGetAddressOf()));


    ////
    // Load precompiled shader blobs from file (automatically compiled via Visual Studio & written to executable directory)

    // Vertex Shader - shader type, shader model, & entrypoint specified in Visual Studio file properties
    ComPtr<ID3DBlob> vsBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"BasicVS.cso", vsBlob.ReleaseAndGetAddressOf()));
    ThrowIfFailed(m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_vertexShader.ReleaseAndGetAddressOf()));

    // Pixel Shader - shader type, shader model, & entrypoint specified in Visual Studio file properties
    ComPtr<ID3DBlob> psBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"BasicPS.cso", psBlob.ReleaseAndGetAddressOf()));
    ThrowIfFailed(m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_pixelShader.ReleaseAndGetAddressOf()));


    ////
    // Declare the vertex layout - MUST align with declared vertex format in the Vertex Shader

    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    ThrowIfFailed(m_device->CreateInputLayout(inputElementDesc, _countof(inputElementDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_inputLayout.ReleaseAndGetAddressOf()));


    ////
    // Create our state which configures the fixed-function graphics pipeline

    // Rasterizer State - solid fill mode & disable primitive culling
    D3D11_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode              = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode              = D3D11_CULL_NONE;
    rasterizerDesc.FrontCounterClockwise = true;

    ThrowIfFailed(m_device->CreateRasterizerState(&rasterizerDesc, m_rasterizerState.ReleaseAndGetAddressOf()));

    // Depth-stencil State - disable depth testing & writes
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable    = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;

    ThrowIfFailed(m_device->CreateDepthStencilState(&depthStencilDesc, m_depthStencilState.ReleaseAndGetAddressOf()));

    // Blend State - disable blend
    D3D11_BLEND_DESC blendDesc{};
    blendDesc.IndependentBlendEnable                = false;
    blendDesc.AlphaToCoverageEnable                 = false;
    blendDesc.RenderTarget[0].BlendEnable           = false;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ThrowIfFailed(m_device->CreateBlendState(&blendDesc, m_blendState.ReleaseAndGetAddressOf()));
}

void D3DApp::Update(float dt)
{
    ////
    // Update the app state

    // Gently auto-rotate the object about the Y-axis
    m_objectRotation += m_objectRotationSpeed * dt;

    // Determine change in mouse position (when the left mouse button is held down)
    XMVECTOR deltaPos = XMLoadFloat2(&m_currPos) - XMLoadFloat2(&m_prevPos);
    m_prevPos = m_currPos;

    m_cameraPhi   -= XMVectorGetY(deltaPos) * m_cameraRotateRate * dt;
    m_cameraTheta += XMVectorGetX(deltaPos) * m_cameraRotateRate * dt;

    m_cameraPhi = std::min(std::max(m_cameraPhi, 5.0f), 175.0f);


    ////
    // Recompute constant buffer data each frame

    const float degToRads = XM_PI / 180.0f;

    // Compute our object's world-space transform
    XMMATRIX scaleMat = XMMatrixScaling(m_objectScale, m_objectScale, m_objectScale);
    XMMATRIX transMat = XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&m_objectPosition));
    XMMATRIX rotMat   = XMMatrixRotationY(m_objectRotation * degToRads);
 
    XMMATRIX worldMat = scaleMat * rotMat * transMat;

    // Compute camera position
    float phi   = m_cameraPhi * degToRads;
    float theta = m_cameraTheta * degToRads;

    // Spherical to Cartesian Coordinates
    XMVECTOR cameraPosition = XMVectorSet(
        m_cameraDistance * sinf(phi) * cosf(theta),
        m_cameraDistance * cosf(phi),
        m_cameraDistance * sinf(phi) * sinf(theta),
        1.0f
    );

    float aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
    XMVECTOR yAxis    = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX viewMat         = XMMatrixLookAtRH(cameraPosition, DirectX::XMLoadFloat3(&m_cameraFocus), yAxis);
    XMMATRIX projMat         = XMMatrixPerspectiveFovRH(DirectX::XMConvertToRadians(60.0f), aspectRatio, 0.25f, 1000.0f); // FOVY, Aspect ratio, Near Plane Z, Far Plane Z
    XMMATRIX worldVieProjMat = worldMat * viewMat * projMat;

    ////
    // Populate our staging buffer

    AppShaderConstants constants {};
    XMStoreFloat4x4(&constants.World, worldMat);
    XMStoreFloat4x4(&constants.WorldViewProjection, worldVieProjMat);

    XMStoreFloat3(&constants.ObjectColor, XMLoadFloat3(&m_objectColor));
    constants.ObjectShininess = m_objectShininess;

    XMStoreFloat4(&constants.LightPositionWS, XMLoadFloat3(&m_lightPosition));
    XMStoreFloat4(&constants.LightColor, XMLoadFloat3(&m_lightColor));

    XMStoreFloat4(&constants.CameraPositionWS, cameraPosition);


    ////
    // Copy the constants to the upload buffer

    // 'Mapping' a resource means the resource memory is mapped to the client's address space to allow application data emplacement
    // Once unmapped it's copied to the GPU-side memory of the upload buffer
    D3D11_MAPPED_SUBRESOURCE subresource;
    ThrowIfFailed(m_deviceContext->Map(m_uploadBuffer.Get(), 0, D3D11_MAP_WRITE, 0, &subresource));

    std::memcpy(subresource.pData, &constants, sizeof(constants));

    m_deviceContext->Unmap(m_uploadBuffer.Get(), 0);
}

void D3DApp::Draw()
{
    // Schedule a copy from the upload buffer to the GPU constant buffer
    // Need to cycle between different constant buffers each frame to avoid writing to one being read by the GPU
    UINT bufferIndex = m_frameIndex & 0x1;
    m_deviceContext->CopyResource(m_constantBuffer[bufferIndex].Get(), m_uploadBuffer.Get());

    // Clear the render target to dark grey
    FLOAT backgroundColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
    m_deviceContext->ClearRenderTargetView(m_backBufferRTV.Get(), backgroundColor);
    m_deviceContext->ClearDepthStencilView(m_depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Bind the color target
    ID3D11RenderTargetView* rtvs[] = { m_backBufferRTV.Get() };
    m_deviceContext->OMSetRenderTargets(1, rtvs, m_depthBufferDSV.Get());

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

    // Bind the vertex and index buffers
    UINT stride = sizeof(PosNormalVertex); // Making a bit of an assumption here of the vertex format in our mesh
    UINT offset = 0;
    ID3D11Buffer* vbuffers[] = { m_vertexBuffer.Get() };

    m_deviceContext->IASetVertexBuffers(0, 1, vbuffers, &stride, &offset);
    m_deviceContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0); // Assuming the format of the index data is 32-bit

    // Set the vertex & pixel shader programs
    m_deviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // Same constant buffer for both shader stages
    ID3D11Buffer* constantBuffers[] = { m_constantBuffer[bufferIndex].Get() };
    m_deviceContext->VSSetConstantBuffers(0, 1, constantBuffers);
    m_deviceContext->PSSetConstantBuffers(0, 1, constantBuffers);

    // Set the fixed-function graphics pipeline state
    m_deviceContext->RSSetState(m_rasterizerState.Get());
    m_deviceContext->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    m_deviceContext->OMSetBlendState(m_blendState.Get(), nullptr, 0xffffffff);

    // Draw the mesh
    m_deviceContext->DrawIndexed(m_indexCount, 0, 0);
}

void D3DApp::Present()
{
    ThrowIfFailed(m_swapChain->Present(1, 0)); // Flip on VBlank (vsync refresh rate interval)
    ++m_frameIndex;
}

LRESULT D3DApp::HandleInput(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        m_isRunning = false;

        return 0;

    case WM_MOUSEMOVE:
    {
        m_currPos.x = static_cast<float>((lParam & 0x0000ffff) >> 0);
        m_currPos.y = static_cast<float>((lParam & 0xffff0000) >> 16);

        if (wParam != MK_LBUTTON)
        {
            m_prevPos = m_currPos;
        }

        return 0;
    }

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
