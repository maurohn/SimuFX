// ============================================================================
// ambient_occlusion.hlsl — Color-based SSAO approximation (PS 3.0)
//
// No depth buffer required. Uses luminance differences between a pixel and
// its 16 surrounding samples to detect "concave" areas (crevices, gaps,
// tire contact, panel lines) and darkens them — simulating occluded ambient
// light that can't reach those spots.
//
// c0: x=pixelW, y=pixelH, z=radius(pixels), w=strength(0-1)
// c1: x=bias(min luma diff), y=unused, z=unused, w=unused
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c0 : register(c0);   // pixelW, pixelH, radius, strength
float4 c1 : register(c1);   // bias, 0, 0, 0

// 16-point uniform ring sample (unit circle)
static const float2 kRing[16] = {
    float2( 1.000,  0.000), float2( 0.924,  0.383),
    float2( 0.707,  0.707), float2( 0.383,  0.924),
    float2( 0.000,  1.000), float2(-0.383,  0.924),
    float2(-0.707,  0.707), float2(-0.924,  0.383),
    float2(-1.000,  0.000), float2(-0.924, -0.383),
    float2(-0.707, -0.707), float2(-0.383, -0.924),
    float2( 0.000, -1.000), float2( 0.383, -0.924),
    float2( 0.707, -0.707), float2( 0.924, -0.383),
};

// Rec.709 luma weights
static const float3 kLuma = float3(0.2126f, 0.7152f, 0.0722f);

float4 PS_AmbientOcclusion(float2 uv : TEXCOORD0) : COLOR
{
    float pixW    = c0.x;
    float pixH    = c0.y;
    float radius  = c0.z;   // sample radius in pixels
    float strength= c0.w;
    float bias    = c1.x;   // minimum luma difference to register as occlusion

    float3 center     = tex2D(ScreenTex, uv).rgb;
    float  centerLuma = dot(center, kLuma);

    // Accumulate neighbor luminance at two radii (inner + outer ring)
    float avgLuma = 0.0f;
    [unroll]
    for (int i = 0; i < 16; ++i) {
        // Alternate between inner (50%) and outer (100%) radius for smoother AO
        float r = (i % 2 == 0) ? radius * 0.50f : radius;
        float2 off = kRing[i] * float2(pixW, pixH) * r;
        float3 s = tex2D(ScreenTex, saturate(uv + off)).rgb;
        avgLuma += dot(s, kLuma);
    }
    avgLuma /= 16.0f;

    // AO factor: how much brighter are the neighbors vs the center?
    // A pixel darker than its surroundings is in a crevice → occlude it.
    float ao = saturate(avgLuma - centerLuma - bias);

    // Don't apply AO to already-bright pixels (car paint highlights, sky)
    // — only to mid/dark tones where shadows should appear.
    float brightSuppress = 1.0f - saturate((centerLuma - 0.45f) * 4.0f);
    ao *= brightSuppress;

    // Apply: darken pixel proportional to AO factor
    float3 result = center * (1.0f - ao * strength);
    return float4(saturate(result), 1.0f);
}
