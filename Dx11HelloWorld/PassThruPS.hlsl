//
// PassThruPS.hlsl
//

// Interpolants from the vertex shader - must align with vertex shader output (done through the semantics, e.g. COLOR0)
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

    float4 Position : SV_Position; // SV_Position - float4
    float3 Color    : COLOR0;      // COLOR0 - arbitrary semantic name used to align data between VS & PS
};

// Pixel shader entrypoint
float4 PSMain(VSInterpolants pin) : SV_Target0
{
    // Just pass the color through directly to the output merger
    return float4(pin.Color, 1);
}
