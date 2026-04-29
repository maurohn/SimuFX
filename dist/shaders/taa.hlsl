// ============================================================================
// taa.hlsl — Temporal Anti-Aliasing (PS 3.0)
//
// Blends current frame with previous frame using neighbourhood AABB clamping
// to prevent ghosting. Pixel size passed explicitly via constants (ddx/ddy
// are unreliable on some PS3.0 implementations).
//
// s0 = current frame
// s1 = previous frame (persistent RT, cleared on device reset)
// c9: x=BlendFactor (0.1-0.2), y=RcpWidth, z=RcpHeight, w=ValidPrev(0/1)
// ============================================================================

sampler2D CurrentTex  : register(s0);
sampler2D PreviousTex : register(s1);

float4 c9 : register(c9);

#define BLEND       c9.x   // Previous frame weight (0.15 = smooth)
#define RCP_W       c9.y   // 1/screenWidth
#define RCP_H       c9.z   // 1/screenHeight
#define VALID_PREV  c9.w   // 1 = prev frame is valid, 0 = first frame (skip blend)

float4 PS_TAA(float2 uv : TEXCOORD0) : COLOR
{
    float3 current  = tex2D(CurrentTex,  uv).rgb;

    // First frame guard — if previous RT is uninitialized, just pass current through
    if (VALID_PREV < 0.5f)
        return float4(current, 1.0f);

    float3 previous = tex2D(PreviousTex, uv).rgb;

    // ---- Neighbourhood clamp: prevent ghosting ----
    // Use explicit pixel size instead of ddx/ddy
    float2 px = float2(RCP_W, RCP_H);

    float3 n0 = tex2D(CurrentTex, clamp(uv + float2( px.x,  0),     0.001f, 0.999f)).rgb;
    float3 n1 = tex2D(CurrentTex, clamp(uv + float2(-px.x,  0),     0.001f, 0.999f)).rgb;
    float3 n2 = tex2D(CurrentTex, clamp(uv + float2( 0,      px.y), 0.001f, 0.999f)).rgb;
    float3 n3 = tex2D(CurrentTex, clamp(uv + float2( 0,     -px.y), 0.001f, 0.999f)).rgb;

    float3 nMin = min(current, min(min(n0, n1), min(n2, n3)));
    float3 nMax = max(current, max(max(n0, n1), max(n2, n3)));

    // Clamp history to local colour box — eliminates ghosting
    float3 clamped = clamp(previous, nMin, nMax);

    // Blend: current frame dominant, history smooths
    float3 result = lerp(current, clamped, BLEND);

    return float4(result, 1.0f);
}
