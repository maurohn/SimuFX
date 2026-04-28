// ============================================================================
// color_boost.hlsl — Saturation + Vibrance + Temperature + Tint
// Constant registers:
//   c2: Saturation, Vibrance, Temperature, Tint
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c2 : register(c2);

#define SAT   c2.x
#define VIB   c2.y
#define TEMP  c2.z
#define TINT  c2.w

float Luma(float3 c) { return dot(c, float3(0.2126f, 0.7152f, 0.0722f)); }

// Saturation: uniform boost of all channels away from grey
float3 AdjustSaturation(float3 col, float sat)
{
    float lum = Luma(col);
    return lerp(float3(lum, lum, lum), col, sat);
}

// Vibrance: boosts less-saturated colours more (protects skin tones)
float3 AdjustVibrance(float3 col, float vib)
{
    float maxC = max(col.r, max(col.g, col.b));
    float minC = min(col.r, min(col.g, col.b));
    float sat  = maxC - minC;
    float lum  = Luma(col);
    float mask = (1.0f - saturate(sat)) * (1.0f - lum);
    return col + (col - float3(lum,lum,lum)) * vib * mask;
}

// Temperature: shift warm (+) / cool (-)
//   Warm = more red + green, less blue
//   Cool = more blue, less red
float3 AdjustTemperature(float3 col, float temp)
{
    col.r += temp * 0.2f;
    col.g += temp * 0.1f;
    col.b -= temp * 0.2f;
    return col;
}

// Tint: shift green (+) / magenta (-)
float3 AdjustTint(float3 col, float tint)
{
    col.g += tint * 0.2f;
    col.r -= tint * 0.1f;
    col.b -= tint * 0.1f;
    return col;
}

float4 PS_Color(float2 uv : TEXCOORD0) : COLOR
{
    float3 col = tex2D(ScreenTex, uv).rgb;

    col = AdjustTemperature(col, TEMP);
    col = AdjustTint(col, TINT);
    col = AdjustSaturation(col, SAT);
    col = AdjustVibrance(col, VIB);

    return float4(saturate(col), 1.0f);
}
