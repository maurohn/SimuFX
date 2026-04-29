// ============================================================================
// final_composite.hlsl — simple pass-through blit (used as identity blit)
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 PS_Final(float2 uv : TEXCOORD0) : COLOR
{
    return tex2D(ScreenTex, uv);
}
