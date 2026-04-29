// ============================================================================
// dof.hlsl — Fake Screen-Space Depth of Field (PS 3.0)
//
// SAFE version: all UV samples clamped, reduced blur radius, early-out for
// sharp zones. Uses UV.y as depth proxy (no depth buffer needed).
//
// - Lower part of screen = near (car, track) = sharp
// - Upper part of screen = far (background)  = blurred
//
// s0 = scene texture
// c10: x=FocusY (0=top, 1=bottom), y=NearBlur, z=FarBlur, w=Transition
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c10 : register(c10);

#define FOCUS_Y     c10.x    // Vertical focus band centre (0-1), default 0.65
#define NEAR_BLUR   c10.y    // Blur strength close to camera (usually 0)
#define FAR_BLUR    c10.z    // Blur strength for distant background
#define TRANSITION  c10.w    // Softness of the focus falloff

// 9-tap Poisson disk blur — cheaper than 7x7 Gaussian, no UV OOB risk with clamp
float3 PoissonBlur(sampler2D tex, float2 uv, float radius)
{
    // 9-tap Poisson disk offsets (normalised to [-1,1])
    const float2 disk[9] = {
        float2( 0.000f,  0.000f),
        float2( 0.707f,  0.000f),
        float2(-0.707f,  0.000f),
        float2( 0.000f,  0.707f),
        float2( 0.000f, -0.707f),
        float2( 0.500f,  0.500f),
        float2(-0.500f,  0.500f),
        float2( 0.500f, -0.500f),
        float2(-0.500f, -0.500f),
    };

    float3 sum = 0;
    for (int i = 0; i < 9; ++i) {
        float2 sampleUV = clamp(uv + disk[i] * radius, 0.001f, 0.999f);
        sum += tex2D(tex, sampleUV).rgb;
    }
    return sum / 9.0f;
}

float4 PS_DoF(float2 uv : TEXCOORD0) : COLOR
{
    float3 sharp  = tex2D(ScreenTex, uv).rgb;

    // Distance from focus band: 0=in-focus, >0=blurred
    float dist   = abs(uv.y - FOCUS_Y);
    float blur_t = smoothstep(0.0f, TRANSITION, dist - 0.05f);

    // Small radius: DoF is a subtle cinematic effect, not a heavy blur
    // blur_t=1 → FAR_BLUR*0.004 UV units → ~4 pixels at 1080p
    float blurRadius = lerp(NEAR_BLUR, FAR_BLUR, blur_t) * 0.004f;

    // Early-out for the sharp zone — save ALU
    if (blurRadius < 0.0003f)
        return float4(sharp, 1.0f);

    float3 blurred = PoissonBlur(ScreenTex, uv, blurRadius);
    float3 result  = lerp(sharp, blurred, blur_t);

    return float4(result, 1.0f);
}
