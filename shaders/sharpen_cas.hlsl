// ============================================================================
// sharpen_cas.hlsl — AMD Contrast Adaptive Sharpening (PS 3.0 port)
// Constant registers:
//   c7: Strength, Clamp, RcpW, RcpH
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c7 : register(c7);

#define STRENGTH c7.x
#define CLAMP_V  c7.y
#define RCP_W    c7.z
#define RCP_H    c7.w

float Luma(float3 rgb) { return dot(rgb, float3(0.299f, 0.587f, 0.114f)); }

float4 PS_SharpenCAS(float2 uv : TEXCOORD0) : COLOR
{
    float2 px = float2(RCP_W, RCP_H);

    float3 a = tex2D(ScreenTex, uv + float2(-1, 0)*px).rgb; // W
    float3 b = tex2D(ScreenTex, uv + float2( 0,-1)*px).rgb; // N
    float3 c = tex2D(ScreenTex, uv).rgb;                     // centre
    float3 d = tex2D(ScreenTex, uv + float2( 1, 0)*px).rgb; // E
    float3 e = tex2D(ScreenTex, uv + float2( 0, 1)*px).rgb; // S

    // CAS sharpening weight
    float minG = min(Luma(a), min(Luma(b), min(Luma(c), min(Luma(d), Luma(e)))));
    float maxG = max(Luma(a), max(Luma(b), max(Luma(c), max(Luma(d), Luma(e)))));

    float w = sqrt(min(minG, 1.0f - maxG) / maxG) * (-0.125f);
    w = max(w, -CLAMP_V);
    w *= STRENGTH;

    float3 result = (w*(a+b+d+e) + c) / (1.0f + 4.0f*w);
    return float4(saturate(result), 1.0f);
}
