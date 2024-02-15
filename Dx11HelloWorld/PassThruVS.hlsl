//--------------------------------------------------------------------------------------
// PassThruVS.hlsl
//
// EA Runtime Technology Group (RTG)
// Copyright (C) Electronic Arts. All rights reserved.
//--------------------------------------------------------------------------------------

// Vertex shader input definition - MUST align with the bound input layout and vertex buffer data format
struct VSInput
{
    // Input semantics (POSITION, COLOR0, TEXCOORD0, etc) are used to align with app-side ID3D11InputLayout declarations
    float3 Position : POSITION;
    float3 Color    : COLOR;
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

    float4 Position : SV_Position;  // SV_Position - float4 - Required; specifies vertex position in NDC space
    float3 Color    : COLOR0;       // COLOR0 - arbitrary semantic name used to align data between VS & PS
};

// Vertex shader entrypoint
VSInterpolants VSMain(VSInput vin)
{
    // Just pass the data through to the primitive assembler
    VSInterpolants vout;
    vout.Position = float4(vin.Position, 1);
    vout.Color = vin.Color;

    return vout;
}