//--------------------------------------------------------------------------------------
// BasicVS.hlsl
//
// EA Runtime Technology Group (RTG)
// Copyright (C) Electronic Arts. All rights reserved.
//--------------------------------------------------------------------------------------

// Vertex shader input definition - MUST align with the bound input layout and vertex buffer data format
struct VSInput
{
    // Input semantics (POSITION, COLOR0, TEXCOORD0, etc) are used to align with app-side ID3D11InputLayout declarations
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
};

// Outputs to be interpolated and fed to pixel shader - must align with pixel shader input
struct VSInterpolants
{
    // Semantics beginning in 'SV_' are special (SV_Position, SV_Target0, etc.)
    // 
    // These are:
    // 1. Outputs used by the graphics pipeline for various purposes
    // 2. Inputs auto-magically supplied by the graphics pipeline to a shader stage
    // 
    // Each stage has a unique set of required/optional input & output semantics which can be leveraged
    // Read more here - https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics

    float4 PositionCS : SV_Position;  // SV_Position - float4 - Required; specifies vertex position in NDC space
    float3 PositionWS : POSITION;     // Arbitrary semantic names used to align data between VS & PS
    float3 NormalWS   : NORMAL;
};


////
// Constant buffers supply global data to the shader
// 
// Note: Data is laid out in strict 32-bit, 4-component fashion
//       So we use float4 instead of float3 for alignment's sake
//       Otherwise we may incur cruel and inexplicable behavior

cbuffer Constants : register(b0)
{
    // Space Transformations - these can't be reused by other objects since its world transform is baked in
    float4x4 World;               // Transform from object to world space
    float4x4 WorldViewProjection; // Transform from object to clip space

    // Object materials
    float3 ObjectColor;
    float  ObjectShininess;

    // Light properties
    float4 LightPositionWS;
    float4 LightColor;

    // Camera properties
    float4 CameraPositionWS;
};


// Vertex shader entrypoint
VSInterpolants VSMain(VSInput vin)
{
    VSInterpolants vout;

    // Transform the vertex position into clip space
    vout.PositionCS = mul(WorldViewProjection, float4(vin.Position, 1));

    // Transform position and normal to world space for pixel shader lighting calculations
    vout.PositionWS = mul(World, float4(vin.Position, 1)).xyz;
    vout.NormalWS   = mul(World, float4(vin.Normal, 0)).xyz;

    return vout;
}