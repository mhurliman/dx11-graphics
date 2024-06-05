//
// BasicPS.hlsl
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

    float4 PositionCS : SV_Position; // SV_Position - float4
    float3 PositionWS : POSITION;      // Arbitrary semantic names used to align data between VS & PS
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


////
// Pixel shader entrypoint
//
// Simple shader which employs diffuse-only Lambertian reflectance lighting from a single point light source

float4 PSMain(VSInterpolants pin) : SV_Target0
{
    float3 V = normalize(CameraPositionWS.xyz - pin.PositionWS); // World-space direction to camera from pixel position

    float3 L = LightPositionWS.xyz - pin.PositionWS; // World-space direction to light from pixel position
    float Ldist = length(L);                         // Pixel distance from light
    L /= Ldist;                                      // Normalize L

    float3 N = normalize(pin.NormalWS); // Renormalize as homogeneous divide & interpolation will denormalize unit vectors

    // Lambertian diffuse
    float NdotL = dot(L, N); 
    float diffuseIntensity = saturate(NdotL);

    // Blinn-phong specular reflectance
    float3 H = normalize(V + L);
    float NdotH = dot(N, H);
    float specularIntensity = pow(saturate(NdotH), ObjectShininess);


    float3 color = (diffuseIntensity + specularIntensity) * LightColor.rgb * ObjectColor.rgb / (Ldist * Ldist);

    // Just pass the color through directly to the output merger
    return float4(color, 1);
}
