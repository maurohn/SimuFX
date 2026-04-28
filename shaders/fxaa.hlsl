// ============================================================================
// fxaa.hlsl — FXAA 3.11 adapted for D3D9 PS 3.0
// Constant registers:
//   c0: rcpWidth, rcpHeight, EdgeThreshold, SubpixelQuality
//   c1: Strength, 0, 0, 0
// ============================================================================

sampler2D ScreenTex : register(s0);

float4 c0 : register(c0);
float4 c1 : register(c1);

#define RCP_W    c0.x
#define RCP_H    c0.y
#define EDGE_TH  c0.z
#define SUBPIX   c0.w
#define STRENGTH c1.x

// Luminance weight
float Luma(float3 rgb) {
    return dot(rgb, float3(0.299f, 0.587f, 0.114f));
}

float4 PS_FXAA(float2 uv : TEXCOORD0) : COLOR
{
    float2 px = float2(RCP_W, RCP_H);

    // Sample centre and neighbours
    float3 rgbM  = tex2D(ScreenTex, uv).rgb;
    float3 rgbN  = tex2D(ScreenTex, uv + float2( 0,-1) * px).rgb;
    float3 rgbS  = tex2D(ScreenTex, uv + float2( 0, 1) * px).rgb;
    float3 rgbE  = tex2D(ScreenTex, uv + float2( 1, 0) * px).rgb;
    float3 rgbW  = tex2D(ScreenTex, uv + float2(-1, 0) * px).rgb;

    float lumM = Luma(rgbM);
    float lumN = Luma(rgbN);
    float lumS = Luma(rgbS);
    float lumE = Luma(rgbE);
    float lumW = Luma(rgbW);

    float lumMin = min(lumM, min(min(lumN, lumS), min(lumE, lumW)));
    float lumMax = max(lumM, max(max(lumN, lumS), max(lumE, lumW)));
    float lumRange = lumMax - lumMin;

    // Skip flat regions
    if (lumRange < max(EDGE_TH, lumMax * 0.125f))
        return float4(rgbM, 1.0f);

    // Diagonal neighbours
    float3 rgbNW = tex2D(ScreenTex, uv + float2(-1,-1) * px).rgb;
    float3 rgbNE = tex2D(ScreenTex, uv + float2( 1,-1) * px).rgb;
    float3 rgbSW = tex2D(ScreenTex, uv + float2(-1, 1) * px).rgb;
    float3 rgbSE = tex2D(ScreenTex, uv + float2( 1, 1) * px).rgb;

    float lumNW = Luma(rgbNW);
    float lumNE = Luma(rgbNE);
    float lumSW = Luma(rgbSW);
    float lumSE = Luma(rgbSE);

    // Subpixel aliasing factor
    float lumAvg = (lumN + lumS + lumE + lumW) * 0.25f;
    float subPixShift = abs(lumAvg - lumM) / lumRange;
    subPixShift = saturate(subPixShift * 3.0f - 1.5f);
    float blendSub = subPixShift * subPixShift * SUBPIX;

    // Edge direction
    float edgeH = abs(lumNW + 2.0f*lumN + lumNE - lumSW - 2.0f*lumS - lumSE);
    float edgeV = abs(lumNW + 2.0f*lumW + lumSW - lumNE - 2.0f*lumE - lumSE);
    bool isHoriz = edgeH >= edgeV;

    // Step along edge
    float2 step = isHoriz ? float2(RCP_W, 0) : float2(0, RCP_H);
    float2 perpStep = isHoriz ? float2(0, RCP_H) : float2(RCP_W, 0);

    float lum1 = isHoriz ? lumS : lumE;
    float lum2 = isHoriz ? lumN : lumW;
    float grad1 = abs(lum1 - lumM);
    float grad2 = abs(lum2 - lumM);

    float2 dir = (grad1 >= grad2) ? perpStep : -perpStep;

    // Blend
    float blendFactor = max(blendSub, 0.0f) * STRENGTH;
    float2 blendUV = uv + dir * blendFactor;

    float3 blended = tex2D(ScreenTex, blendUV).rgb;
    float3 result  = lerp(rgbM, blended, blendFactor);

    return float4(result, 1.0f);
}
